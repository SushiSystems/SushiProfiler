/**************************************************************************/
/* overhead_analysis.cpp                                                  */
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
 * @file overhead_analysis.cpp
 * @brief Counts what the run recorded and lost, and compares its period times with a baseline.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/overhead_analysis.hpp>

#include <string>
#include <utility>

#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/statistics/sorted_samples.hpp>

#include "record_queries.hpp"

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns how many events the capture path wrote for @p record.
        std::uint64_t events_of(const RunRecord& record) noexcept
        {
            std::uint64_t events = 0U;
            for (const PeriodRecord& period : record.periods)
            {
                events += period.budget_ns != 0U ? 2U : 1U;
                for (const ZoneTree& tree : period.zones)
                {
                    for (const ZoneNode& node : tree.nodes)
                        events += node.calls;
                }
                for (const SampleSeries& series : period.samples)
                    events += series.time_ns.size();
            }
            return events;
        }

        /// @brief Appends the row @p metric with @p value.
        void add_row(Table& table, std::string metric, double value)
        {
            table.rows.push_back(Row{Cell::text(std::move(metric)), Cell::real(value)});
        }

        /// @brief Returns the median active period time of the timeline named @p name, or 0.
        double median_of(const RunRecord& record, std::string_view name)
        {
            const NameID timeline = record.find_name(name);
            if (timeline == 0U)
                return 0.0;
            const std::vector<double> lengths =
                Detail::durations(Detail::active_periods(record, timeline));
            return lengths.empty() ? 0.0 : SortedSamples(lengths).median();
        }
    } // namespace

    std::string_view OverheadAnalysis::name() const noexcept
    {
        return "overhead";
    }

    Table OverheadAnalysis::run(const RunRecord& record, const AnalysisOptions& options) const
    {
        Table table;
        table.name = "overhead";
        table.add_column("metric");
        table.add_column("value");
        std::uint64_t dropped = 0U;
        std::uint64_t late = 0U;
        for (const ThreadRecord& thread : record.threads)
        {
            dropped += thread.dropped;
            late += thread.late;
        }
        const std::uint64_t events = events_of(record);
        add_row(table, "events", static_cast<double>(events));
        add_row(table, "bytes", static_cast<double>(events * sizeof(TraceEvent)));
        add_row(table, "dropped", static_cast<double>(dropped));
        add_row(table, "late", static_cast<double>(late));
        add_row(table, "outside_periods", static_cast<double>(record.drops.outside_periods));
        add_row(table, "open_at_stop", static_cast<double>(record.drops.open_at_stop));
        add_row(table, "spans_dropped", static_cast<double>(record.drops.spans_dropped));
        add_row(table, "clock_read_ns", record.clock.read_cost_ns);
        if (options.baseline == nullptr)
        {
            table.notes.emplace_back("No baseline record was given, so the cost is not measured.");
            return table;
        }
        for (const NameID timeline : Detail::active_timelines(record))
        {
            const std::string name(record.name_of(timeline));
            const double profiled = median_of(record, name);
            const double baseline = median_of(*options.baseline, name);
            if (baseline == 0.0)
                continue;
            add_row(table, "baseline_median_ns:" + name, baseline);
            add_row(table, "median_ns:" + name, profiled);
            add_row(table, "cost:" + name, (profiled - baseline) / baseline);
        }
        return table;
    }
} // namespace SushiProfiler
