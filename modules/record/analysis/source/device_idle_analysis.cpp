/**************************************************************************/
/* device_idle_analysis.cpp                                               */
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
 * @file device_idle_analysis.cpp
 * @brief Merges each track's spans per period into busy segments and classifies the gaps.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/device_idle_analysis.hpp>

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <SushiProfiler/analysis/sample_summary.hpp>

#include "record_queries.hpp"

namespace SushiProfiler
{
    namespace
    {
        /// @brief The label of a gap the device waited out with no host zone to blame.
        constexpr const char* K_UNATTRIBUTED = "unattributed";

        /// @brief The label of a gap whose next span was already queued when the gap began.
        constexpr const char* K_LAUNCH = "launch";

        /// @brief Idle time of one track in one period, split by cause and by blamed zone.
        struct IdleTime
        {
            std::uint64_t launch_ns = 0U; /**< Gaps whose next span was already queued. */
            std::uint64_t host_ns = 0U;   /**< Gaps the device waited for a submit. */
            std::map<std::string, std::uint64_t> blamed_ns; /**< Idle time by blamed label. */
        };

        /// @brief One track's totals over the active periods.
        struct TrackTotals
        {
            std::vector<double> idle;     /**< Idle time per period. */
            std::uint64_t launch_ns = 0U; /**< Launch idle over the periods. */
            std::uint64_t host_ns = 0U;   /**< Host idle over the periods. */
            std::map<std::string, std::uint64_t> blamed_ns; /**< Idle time by blamed label. */
        };

        /// @brief Returns the label of the innermost zone of @p thread open at @p time, or empty.
        std::string innermost_zone(const RunRecord& record, const PeriodRecord& period,
                                   std::uint32_t thread, std::uint64_t time)
        {
            const ZoneRun* found = nullptr;
            for (const ZoneRun& run : period.runs)
            {
                if (run.thread != thread || run.begin_ns > time || time >= run.end_ns)
                    continue;
                if (found == nullptr || run.depth > found->depth ||
                    (run.depth == found->depth && run.begin_ns >= found->begin_ns))
                    found = &run;
            }
            if (found == nullptr)
                return std::string();
            ZoneNode node;
            node.site = found->site;
            node.name = found->name;
            return Detail::node_label(record, node);
        }

        /// @brief Returns the label a gap from @p time is charged to: the host zone open on
        /// @p thread, else launch when the next span was queued, else unattributed.
        std::string blamed_label(const RunRecord& record, const PeriodRecord& period,
                                 const std::optional<std::uint32_t>& thread, std::uint64_t time,
                                 bool launch)
        {
            if (thread)
            {
                std::string zone = innermost_zone(record, period, *thread, time);
                if (!zone.empty())
                    return zone;
            }
            return launch ? K_LAUNCH : K_UNATTRIBUTED;
        }

        /// @brief Returns the spans of @p track in @p period, clipped to it, by start time.
        std::vector<Span> spans_on(const PeriodRecord& period, TrackID track)
        {
            std::vector<Span> spans;
            for (std::size_t row = 0U; row < period.spans.size(); ++row)
            {
                Span span = period.spans.span(row);
                if (span.track != track || span.end_ns <= period.begin_ns ||
                    span.start_ns >= period.end_ns)
                    continue;
                span.start_ns = std::max(span.start_ns, period.begin_ns);
                span.end_ns = std::min(span.end_ns, period.end_ns);
                spans.push_back(span);
            }
            std::stable_sort(spans.begin(), spans.end(), [](const Span& left, const Span& right)
                             {
                                 return left.start_ns < right.start_ns;
                             });
            return spans;
        }

        /// @brief Returns the idle time of @p spans in @p period, each gap given its cause.
        IdleTime idle_of(const RunRecord& record, const PeriodRecord& period,
                         const std::vector<Span>& spans)
        {
            IdleTime idle;
            std::uint64_t busy_until = period.begin_ns;
            std::optional<std::uint32_t> last_thread;
            for (const Span& span : spans)
            {
                if (span.start_ns > busy_until)
                {
                    const std::uint64_t gap = span.start_ns - busy_until;
                    const bool launch = span.queued_ns <= busy_until;
                    if (launch)
                        idle.launch_ns += gap;
                    else
                        idle.host_ns += gap;
                    idle.blamed_ns[blamed_label(record, period, span.submit_thread, busy_until,
                                                launch)] += gap;
                }
                busy_until = std::max(busy_until, span.end_ns);
                last_thread = span.submit_thread;
            }
            const std::uint64_t tail = period.end_ns - busy_until;
            idle.host_ns += tail;
            if (tail > 0U)
                idle.blamed_ns[blamed_label(record, period, last_thread, busy_until, false)] +=
                    tail;
            return idle;
        }

        /// @brief Returns the label with the most idle time in @p blamed and that time, or empty.
        std::pair<std::string, std::uint64_t> top_blame(
            const std::map<std::string, std::uint64_t>& blamed)
        {
            std::pair<std::string, std::uint64_t> top;
            for (const auto& [label, idle] : blamed)
            {
                if (idle > top.second)
                    top = {label, idle};
            }
            return top;
        }
    } // namespace

    std::string_view DeviceIdleAnalysis::name() const noexcept
    {
        return "device_idle";
    }

    Table DeviceIdleAnalysis::run(const RunRecord& record, const AnalysisOptions& options) const
    {
        Table table;
        table.name = "device_idle";
        table.add_column("track");
        add_summary_columns(table, Unit::Nanoseconds);
        table.add_column("idle_share", Unit::Ratio);
        table.add_column("launch_share", Unit::Ratio);
        table.add_column("host_share", Unit::Ratio);
        table.add_column("blamed_zone");
        table.add_column("blamed_share", Unit::Ratio);
        const std::vector<const PeriodRecord*> periods = Detail::active_periods(record);
        std::map<TrackID, TrackTotals> tracks;
        for (const PeriodRecord* const period : periods)
        {
            for (std::size_t row = 0U; row < period->spans.size(); ++row)
                tracks.emplace(period->spans.span(row).track, TrackTotals{});
        }
        double length = 0.0;
        for (const PeriodRecord* const period : periods)
            length += static_cast<double>(period->duration_ns());
        for (auto& [track, totals] : tracks)
        {
            for (const PeriodRecord* const period : periods)
            {
                const IdleTime idle = idle_of(record, *period, spans_on(*period, track));
                totals.idle.push_back(static_cast<double>(idle.launch_ns + idle.host_ns));
                totals.launch_ns += idle.launch_ns;
                totals.host_ns += idle.host_ns;
                for (const auto& [label, blamed] : idle.blamed_ns)
                    totals.blamed_ns[label] += blamed;
            }
            const auto idle = static_cast<double>(totals.launch_ns + totals.host_ns);
            const std::string label = track < record.tracks.size() && !record.tracks[track].empty()
                                          ? record.tracks[track]
                                          : "track " + std::to_string(track);
            Row row{Cell::text(label)};
            append_summary(row, summarise(totals.idle, options.bootstrap));
            row.push_back(Cell::real(length > 0.0 ? idle / length : 0.0));
            const double launch = static_cast<double>(totals.launch_ns);
            const double host = static_cast<double>(totals.host_ns);
            row.push_back(Cell::real(idle > 0.0 ? launch / idle : 0.0));
            row.push_back(Cell::real(idle > 0.0 ? host / idle : 0.0));
            const std::pair<std::string, std::uint64_t> top = top_blame(totals.blamed_ns);
            row.push_back(Cell::text(top.first));
            row.push_back(Cell::real(idle > 0.0 ? static_cast<double>(top.second) / idle : 0.0));
            table.rows.push_back(std::move(row));
        }
        return table;
    }
} // namespace SushiProfiler
