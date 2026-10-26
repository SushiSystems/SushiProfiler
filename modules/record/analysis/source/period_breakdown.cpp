/**************************************************************************/
/* period_breakdown.cpp                                                   */
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
 * @file period_breakdown.cpp
 * @brief Sums a period's zone trees into self time per group, uncovered time and thread totals.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/period_breakdown.hpp>

#include <algorithm>
#include <map>

namespace SushiProfiler
{
    std::string group_of_node(const std::vector<SiteRecord>& sites, const ZoneTree& tree,
                              std::size_t node)
    {
        std::int32_t at = static_cast<std::int32_t>(node);
        while (at >= 0)
        {
            const ZoneNode& current = tree.nodes[static_cast<std::size_t>(at)];
            if (current.site < sites.size() && !sites[current.site].group.empty())
                return sites[current.site].group;
            at = current.parent;
        }
        return K_UNGROUPED_GROUP;
    }

    PeriodBreakdown break_down_period(const PeriodRecord& period,
                                      const std::vector<SiteRecord>& sites)
    {
        std::map<std::string, std::uint64_t> self_by_group;
        std::map<std::uint32_t, ThreadTotal> by_thread;
        std::uint64_t covered = 0U;
        for (const ZoneTree& tree : period.zones)
        {
            ThreadTotal& total = by_thread[tree.thread];
            total.thread = tree.thread;
            const bool marking = tree.thread == period.thread;
            for (std::size_t index = 0U; index < tree.nodes.size(); ++index)
            {
                const ZoneNode& node = tree.nodes[index];
                total.self_ns += node.self_ns;
                if (node.parent < 0)
                    total.root_inclusive_ns += node.inclusive_ns;
                if (!marking)
                    continue;
                self_by_group[group_of_node(sites, tree, index)] += node.self_ns;
                if (node.parent < 0)
                    covered += node.inclusive_ns;
            }
        }
        PeriodBreakdown breakdown;
        for (const auto& [group, self_ns] : self_by_group)
            breakdown.groups.push_back(GroupSelfTime{group, self_ns});
        std::stable_sort(breakdown.groups.begin(), breakdown.groups.end(),
                         [](const GroupSelfTime& left, const GroupSelfTime& right)
                         {
                             return left.self_ns > right.self_ns;
                         });
        const std::uint64_t duration = period.duration_ns();
        breakdown.uncovered_ns = duration > covered ? duration - covered : 0U;
        for (const auto& entry : by_thread)
            breakdown.threads.push_back(entry.second);
        return breakdown;
    }
} // namespace SushiProfiler
