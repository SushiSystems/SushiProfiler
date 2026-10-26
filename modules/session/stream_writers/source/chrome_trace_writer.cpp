/**************************************************************************/
/* chrome_trace_writer.cpp                                                */
/**************************************************************************/
/*                          This file is part of:                         */
/*                              SushiProfiler                             */
/*              https://github.com/sushisystems/sushiprofiler             */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the PolyForm Noncommercial License 1.0.0 (the           */
/* "License"); you may not use this file except in compliance with the    */
/* License. You may obtain a copy of the License at                       */
/*                                                                        */
/*     https://polyformproject.org/licenses/noncommercial/1.0.0           */
/*                                                                        */
/* Noncommercial use is free. Commercial use requires a separate licence  */
/* from Sushi Systems; see COMMERCIAL.md. The software is provided        */
/* "as is", without warranty of any kind.                                 */
/**************************************************************************/

/**
 * @file chrome_trace_writer.cpp
 * @brief Writes zone runs, samples, flows and spans as Chrome trace events, one per line.
 * @author Mustafa Garip
 * @see modules/session/stream_writers/README.md
 */

#include <SushiProfiler/stream_writers/chrome_trace_writer.hpp>

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

    ChromeTraceWriter::ChromeTraceWriter(std::string path) : path_(std::move(path))
    {

    }

    WriteStatus ChromeTraceWriter::begin(const RunRecord& record)
    {
        static_cast<void>(record);
        file_.open(path_, std::ios::binary | std::ios::trunc);
        if (!file_)
            return WriteStatus::PathUnwritable;
        file_ << "{\"displayTimeUnit\": \"ns\", \"traceEvents\": [";
        emit(name_event("process_name", K_HOST_PROCESS, 0, "host"));
        emit(name_event("process_name", K_DEVICE_PROCESS, 0, "devices"));
        return status();
    }

    WriteStatus ChromeTraceWriter::write_period(const RunRecord& record,
                                                const PeriodRecord& period,
                                                const std::vector<ZoneRun>& instances)
    {
        if (!file_.is_open())
            return WriteStatus::StreamFailed;
        JsonWriter marker(false);
        const std::string label = std::string(record.name_of(period.timeline)) + " #" +
                                  std::to_string(period.index);
        open_event(marker, "X", label, K_HOST_PROCESS, timeline_row(record, period.timeline),
                   period.begin_ns);
        marker.key("dur");
        marker.real(microseconds(period.duration_ns()));
        marker.key("args");
        marker.begin_object(JsonLayout::Inline);
        marker.key("warmup");
        marker.boolean(period.warmup);
        marker.end_object();
        marker.end_object();
        emit(marker.text());
        for (const ZoneRun& instance : instances)
        {
            const SiteRecord* const site = record.site(instance.site);
            const std::string name = instance.name != 0U
                                         ? std::string(record.name_of(instance.name))
                                         : site_label(site);
            JsonWriter out(false);
            open_event(out, "X", name, K_HOST_PROCESS,
                       host_row(record, instance.thread, instance.lane), instance.begin_ns);
            out.key("dur");
            out.real(microseconds(instance.end_ns - instance.begin_ns));
            out.key("cat");
            out.string(site == nullptr || site->group.empty() ? "zone" : site->group);
            out.end_object();
            emit(out.text());
        }
        write_samples(record, period);
        write_spans(record, period);
        return status();
    }

    WriteStatus ChromeTraceWriter::end(const RunRecord& record)
    {
        static_cast<void>(record);
        if (!file_.is_open())
            return WriteStatus::StreamFailed;
        file_ << "\n]}\n";
        file_.close();
        return file_ ? WriteStatus::Written : WriteStatus::StreamFailed;
    }

    void ChromeTraceWriter::emit(std::string_view event)
    {
        file_ << (first_ ? "\n" : ",\n") << event;
        first_ = false;
    }

    std::int64_t ChromeTraceWriter::host_row(const RunRecord& record, std::uint32_t thread,
                                             NameID lane)
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
        emit(name_event("thread_name", K_HOST_PROCESS, row, name));
        return row;
    }

    std::int64_t ChromeTraceWriter::timeline_row(const RunRecord& record, NameID timeline)
    {
        const auto found = rows_.find({K_TIMELINE_THREAD, timeline});
        if (found != rows_.end())
            return found->second;
        const std::int64_t row = next_row_++;
        rows_.emplace(std::make_pair(K_TIMELINE_THREAD, timeline), row);
        emit(name_event("thread_name", K_HOST_PROCESS, row,
                        "timeline " + std::string(record.name_of(timeline))));
        return row;
    }

    void ChromeTraceWriter::name_track(const RunRecord& record, TrackID track)
    {
        if (!named_tracks_.insert(track).second)
            return;
        const std::string name =
            track < record.tracks.size() ? record.tracks[track] : "track " + std::to_string(track);
        emit(name_event("thread_name", K_DEVICE_PROCESS, static_cast<std::int64_t>(track), name));
    }

    void ChromeTraceWriter::write_samples(const RunRecord& record, const PeriodRecord& period)
    {
        for (const SampleSeries& series : period.samples)
        {
            const SiteRecord* const site = record.site(series.site);
            const SiteRole role = site == nullptr ? SiteRole::Instant : site->role;
            const std::int64_t row = host_row(record, series.thread, 0U);
            for (std::size_t index = 0U; index < series.time_ns.size(); ++index)
            {
                JsonWriter out(false);
                if (role == SiteRole::Counter || role == SiteRole::Value)
                {
                    open_event(out, "C", site_label(site), K_HOST_PROCESS, row,
                               series.time_ns[index]);
                    out.key("args");
                    out.begin_object(JsonLayout::Inline);
                    out.key("value");
                    out.real(series.values[index]);
                    out.end_object();
                }
                else if (role == SiteRole::Flow)
                {
                    const FlowPoint point = series.points[index];
                    const char* const phase = point == FlowPoint::Begin  ? "s"
                                              : point == FlowPoint::Step ? "t"
                                                                         : "f";
                    open_event(out, phase, site_label(site), K_HOST_PROCESS, row,
                               series.time_ns[index]);
                    out.key("cat");
                    out.string("flow");
                    out.key("id");
                    out.unsigned_integer(series.items[index]);
                    out.key("bp");
                    out.string("e");
                }
                else
                {
                    open_event(out, "i", site_label(site), K_HOST_PROCESS, row,
                               series.time_ns[index]);
                    out.key("s");
                    out.string("t");
                }
                out.end_object();
                emit(out.text());
            }
        }
    }

    void ChromeTraceWriter::write_spans(const RunRecord& record, const PeriodRecord& period)
    {
        for (std::size_t row = 0U; row < period.spans.size(); ++row)
        {
            const Span span = period.spans.span(row);
            name_track(record, span.track);
            JsonWriter out(false);
            open_event(out, "X", record.name_of(span.name), K_DEVICE_PROCESS,
                       static_cast<std::int64_t>(span.track), span.start_ns);
            out.key("dur");
            out.real(microseconds(span.end_ns - span.start_ns));
            out.key("args");
            out.begin_object(JsonLayout::Inline);
            out.key("queued_us");
            out.real(microseconds(span.queued_ns));
            write_attributes(out, record, period.spans, row);
            out.end_object();
            out.end_object();
            emit(out.text());
        }
    }

    WriteStatus ChromeTraceWriter::status() const
    {
        return file_ ? WriteStatus::Written : WriteStatus::StreamFailed;
    }
} // namespace SushiProfiler
