/**************************************************************************/
/* breakdown_analysis.cpp                                                 */
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
 * @file breakdown_analysis.cpp
 * @brief Sums self time per group on each marking thread and summarises it over active periods.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/breakdown_analysis.hpp>

#include <algorithm>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <SushiProfiler/analysis/sample_summary.hpp>

#include "record_queries.hpp"

namespace SushiProfiler
{
    std::string_view BreakdownAnalysis::name() const noexcept
    {
        return "breakdown";
    }

    Table BreakdownAnalysis::run(const RunRecord& record, const AnalysisOptions& options) const
    {
        Table table;
        table.name = "breakdown";
        table.add_column("timeline");
        table.add_column("group");
        add_summary_columns(table, Unit::Nanoseconds);
        table.add_column("share", Unit::Ratio);
        for (const NameID timeline : Detail::active_timelines(record))
        {
            const std::vector<const PeriodRecord*> periods =
                Detail::active_periods(record, timeline);
            const Detail::GroupSeries series = Detail::group_series(record, periods);
            const std::vector<double> lengths = Detail::durations(periods);
            const double length = std::accumulate(lengths.begin(), lengths.end(), 0.0);
            std::vector<std::pair<std::string, SampleSummary>> rows;
            for (const auto& [group, times] : series.groups)
                rows.emplace_back(group, summarise(times, options.bootstrap));
            std::stable_sort(rows.begin(), rows.end(), [](const auto& left, const auto& right)
                             {
                                 return left.second.median > right.second.median;
                             });
            rows.emplace_back(Detail::K_UNCOVERED_LABEL,
                              summarise(series.uncovered, options.bootstrap));
            for (const auto& [group, summary] : rows)
            {
                const bool uncovered = group == Detail::K_UNCOVERED_LABEL;
                const std::vector<double>& times =
                    uncovered ? series.uncovered : series.groups.at(group);
                const double total = std::accumulate(times.begin(), times.end(), 0.0);
                Row row{Cell::text(std::string(record.name_of(timeline))), Cell::text(group)};
                append_summary(row, summary);
                row.push_back(Cell::real(length > 0.0 ? total / length : 0.0));
                table.rows.push_back(std::move(row));
            }
        }
        return table;
    }
} // namespace SushiProfiler
