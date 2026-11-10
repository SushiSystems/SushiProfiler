/**************************************************************************/
/* chrome_trace_events.cpp                                                */
/**************************************************************************/
/*                          This file is part of:                         */
/*                              SushiProfiler                             */
/*              https://github.com/sushisystems/sushiprofiler             */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file chrome_trace_events.cpp
 * @brief Builds zone runs, samples, flows and spans as Chrome trace events, one JSON text each.
 * @author Mustafa Garip
 * @see modules/session/stream_writers/README.md
 */

#include <SushiProfiler/stream_writers/chrome_trace_events.hpp>

#include <limits>

#include <SushiProfiler/json/json_writer.hpp>
#include <SushiProfiler/sites/site.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief The process of host rows.
        constexpr std::int64_t K_HOST_PROCESS = 1;

        /// @brief The process of device track rows.
        constexpr std::int64_t K_DEVICE_PROCESS = 2;

        /// @brief The thread a timeline row is keyed by.
        constexpr std::uint32_t K_TIMELINE_THREAD = std::numeric_limits<std::uint32_t>::max();

        /// @brief Returns @p ns in the trace's microseconds.
        double microseconds(std::uint64_t ns) noexcept
        {
            return static_cast<double>(ns) / 1000.0;
        }

        /// @brief Opens an event of phase @p phase named @p name on row @p pid, @p tid at @p ns.
        void open_event(JsonWriter& out, std::string_view phase, std::string_view name,
                        std::int64_t pid, std::int64_t tid, std::uint64_t ns)
        {
            out.begin_object(JsonLayout::Inline);
            out.key("ph");
            out.string(phase);
            out.key("name");
            out.string(name);
            out.key("pid");
            out.integer(pid);
            out.key("tid");
            out.integer(tid);
            out.key("ts");
            out.real(microseconds(ns));
        }

        /// @brief Returns a metadata event naming process @p pid or its row @p tid.
        std::string name_event(std::string_view what, std::int64_t pid, std::int64_t tid,
                               std::string_view name)
        {
            JsonWriter out(false);
            open_event(out, "M", what, pid, tid, 0U);
            out.key("args");
            out.begin_object(JsonLayout::Inline);
            out.key("name");
            out.string(name);
            out.end_object();
            out.end_object();
            return out.take();
        }

        /// @brief Returns the name a site's event shows: its own, with its unit when it has one.
        std::string site_label(const SiteRecord* site)
        {
            if (site == nullptr)
                return "unknown site";
            return site->unit.empty() ? site->name : site->name + " (" + site->unit + ")";
        }

        /// @brief Writes span @p row's attributes into @p out's open object.
        void write_attributes(JsonWriter& out, const RunRecord& record, const SpanTable& spans,
                              std::size_t row)
        {
            for (const AttributeColumn& column : spans.attributes())
            {
                if (column.present[row] == 0U)
                    continue;
                out.key(record.name_of(column.key));
                if (column.kind == AttributeKind::Integer)
                    out.integer(column.integers[row]);
                else if (column.kind == AttributeKind::Real)
                    out.real(column.reals[row]);
                else
                    out.string(record.name_of(static_cast<NameID>(column.integers[row])));
            }
        }
    } // namespace

    std::vector<std::string> ChromeTraceEvents::opening() const
    {
        return {name_event("process_name", K_HOST_PROCESS, 0, "host"),
                name_event("process_name", K_DEVICE_PROCESS, 0, "devices")};
    }

    std::vector<std::string> ChromeTraceEvents::events_of(const RunRecord& record,
                                                          const PeriodRecord& period,
                                                          const std::vector<ZoneRun>& instances)
    {
        std::vector<std::string> out;
        JsonWriter marker(false);
        const std::string label = std::string(record.name_of(period.timeline)) + " #" +
                                  std::to_string(period.index);
        open_event(marker, "X", label, K_HOST_PROCESS, timeline_row(record, period.timeline, out),
                   period.begin_ns);
        marker.key("dur");
        marker.real(microseconds(period.duration_ns()));
        marker.key("args");
        marker.begin_object(JsonLayout::Inline);
        marker.key("warmup");
        marker.boolean(period.warmup);
        marker.end_object();
        marker.end_object();
        out.push_back(marker.text());
        for (const ZoneRun& instance : instances)
        {
            const SiteRecord* const site = record.site(instance.site);
            const std::string name = instance.name != 0U
                                         ? std::string(record.name_of(instance.name))
                                         : site_label(site);
            JsonWriter zone(false);
            open_event(zone, "X", name, K_HOST_PROCESS,
                       host_row(record, instance.thread, instance.lane, out), instance.begin_ns);
            zone.key("dur");
            zone.real(microseconds(instance.end_ns - instance.begin_ns));
            zone.key("cat");
            zone.string(site == nullptr || site->group.empty() ? "zone" : site->group);
            zone.end_object();
            out.push_back(zone.text());
        }
        add_samples(record, period, out);
        add_spans(record, period, out);
        return out;
    }

    std::int64_t ChromeTraceEvents::host_row(const RunRecord& record, std::uint32_t thread,
                                             NameID lane, std::vector<std::string>& out)
    {
        const auto found = rows_.find({thread, lane});
        if (found != rows_.end())
            return found->second;
        std::string name = "thread " + std::to_string(thread);
        for (const ThreadRecord& known : record.threads)
        {
            if (known.index == thread && !known.name.empty())
                name = known.name;
        }
        const std::int64_t row = lane == 0U ? static_cast<std::int64_t>(thread) : next_row_++;
        if (lane != 0U)
            name += " / " + std::string(record.name_of(lane));
        rows_.emplace(std::make_pair(thread, lane), row);
        out.push_back(name_event("thread_name", K_HOST_PROCESS, row, name));
        return row;
    }

    std::int64_t ChromeTraceEvents::timeline_row(const RunRecord& record, NameID timeline,
                                                 std::vector<std::string>& out)
    {
        const auto found = rows_.find({K_TIMELINE_THREAD, timeline});
        if (found != rows_.end())
            return found->second;
        const std::int64_t row = next_row_++;
        rows_.emplace(std::make_pair(K_TIMELINE_THREAD, timeline), row);
        out.push_back(name_event("thread_name", K_HOST_PROCESS, row,
                                 "timeline " + std::string(record.name_of(timeline))));
        return row;
    }

    void ChromeTraceEvents::name_track(const RunRecord& record, TrackID track,
                                       std::vector<std::string>& out)
    {
        if (!named_tracks_.insert(track).second)
            return;
        const std::string name =
            track < record.tracks.size() ? record.tracks[track] : "track " + std::to_string(track);
        out.push_back(name_event("thread_name", K_DEVICE_PROCESS,
                                 static_cast<std::int64_t>(track), name));
    }

    void ChromeTraceEvents::add_samples(const RunRecord& record, const PeriodRecord& period,
                                        std::vector<std::string>& out)
    {
        for (const SampleSeries& series : period.samples)
        {
            const SiteRecord* const site = record.site(series.site);
            const SiteRole role = site == nullptr ? SiteRole::Instant : site->role;
            const std::int64_t row = host_row(record, series.thread, 0U, out);
            for (std::size_t index = 0U; index < series.time_ns.size(); ++index)
            {
                JsonWriter event(false);
                if (role == SiteRole::Counter || role == SiteRole::Value)
                {
                    open_event(event, "C", site_label(site), K_HOST_PROCESS, row,
                               series.time_ns[index]);
                    event.key("args");
                    event.begin_object(JsonLayout::Inline);
                    event.key("value");
                    event.real(series.values[index]);
                    event.end_object();
                }
                else if (role == SiteRole::Flow)
                {
                    const FlowPoint point = series.points[index];
                    const char* const phase = point == FlowPoint::Begin  ? "s"
                                              : point == FlowPoint::Step ? "t"
                                                                         : "f";
                    open_event(event, phase, site_label(site), K_HOST_PROCESS, row,
                               series.time_ns[index]);
                    event.key("cat");
                    event.string("flow");
                    event.key("id");
                    event.unsigned_integer(series.items[index]);
                    event.key("bp");
                    event.string("e");
                }
                else
                {
                    open_event(event, "i", site_label(site), K_HOST_PROCESS, row,
                               series.time_ns[index]);
                    event.key("s");
                    event.string("t");
                }
                event.end_object();
                out.push_back(event.text());
            }
        }
    }

    void ChromeTraceEvents::add_spans(const RunRecord& record, const PeriodRecord& period,
                                      std::vector<std::string>& out)
    {
        for (std::size_t row = 0U; row < period.spans.size(); ++row)
        {
            const Span span = period.spans.span(row);
            name_track(record, span.track, out);
            JsonWriter event(false);
            open_event(event, "X", record.name_of(span.name), K_DEVICE_PROCESS,
                       static_cast<std::int64_t>(span.track), span.start_ns);
            event.key("dur");
            event.real(microseconds(span.end_ns - span.start_ns));
            event.key("args");
            event.begin_object(JsonLayout::Inline);
            event.key("queued_us");
            event.real(microseconds(span.queued_ns));
            write_attributes(event, record, period.spans, row);
            event.end_object();
            event.end_object();
            out.push_back(event.text());
        }
    }
} // namespace SushiProfiler
