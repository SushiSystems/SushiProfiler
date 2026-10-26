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
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <SushiProfiler/analysis/sample_summary.hpp>

#include "record_queries.hpp"

namespace SushiProfiler
{
    namespace
    {
        /// @brief The label of the time no zone covered.
        constexpr const char* K_UNCOVERED = "(uncovered)";

        /// @brief One group's self time per period, and its total over them.
        struct GroupTimes
        {
            std::vector<double> per_period; /**< Self time per active period. */
            double total = 0.0;             /**< The sum over the periods. */
        };

        /// @brief Adds the self time of each group in @p period's marking-thread trees.
        void add_period(const RunRecord& record, const PeriodRecord& period, std::size_t slot,
                        std::map<std::string, GroupTimes>& groups, double& covered)
        {
            for (const ZoneTree& tree : period.zones)
            {
                if (tree.thread != period.thread)
                    continue;
                for (std::size_t index = 0U; index < tree.nodes.size(); ++index)
                {
                    const ZoneNode& node = tree.nodes[index];
                    GroupTimes& group = groups[Detail::group_of(record, tree, index)];
                    group.per_period.resize(slot + 1U, 0.0);
                    group.per_period[slot] += static_cast<double>(node.self_ns);
                    if (node.parent < 0)
                        covered += static_cast<double>(node.inclusive_ns);
                }
            }
        }
    } // namespace

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
            std::map<std::string, GroupTimes> groups;
            GroupTimes uncovered;
            double length = 0.0;
            for (std::size_t slot = 0U; slot < periods.size(); ++slot)
            {
                double covered = 0.0;
                add_period(record, *periods[slot], slot, groups, covered);
                const auto duration = static_cast<double>(periods[slot]->duration_ns());
                uncovered.per_period.push_back(std::max(0.0, duration - covered));
                length += duration;
            }
            std::vector<std::pair<std::string, SampleSummary>> rows;
            for (auto& [group, times] : groups)
            {
                times.per_period.resize(periods.size(), 0.0);
                for (const double value : times.per_period)
                    times.total += value;
                rows.emplace_back(group, summarise(times.per_period, options.bootstrap));
            }
            std::stable_sort(rows.begin(), rows.end(), [](const auto& left, const auto& right)
                             {
                                 return left.second.median > right.second.median;
                             });
            for (const double value : uncovered.per_period)
                uncovered.total += value;
            rows.emplace_back(K_UNCOVERED, summarise(uncovered.per_period, options.bootstrap));
            for (const auto& [group, summary] : rows)
            {
                const double total =
                    group == K_UNCOVERED ? uncovered.total : groups.at(group).total;
                Row row{Cell::text(std::string(record.name_of(timeline))), Cell::text(group)};
                append_summary(row, summary);
                row.push_back(Cell::real(length > 0.0 ? total / length : 0.0));
                table.rows.push_back(std::move(row));
            }
        }
        return table;
    }
} // namespace SushiProfiler
