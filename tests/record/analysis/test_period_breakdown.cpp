/**************************************************************************/
/* test_period_breakdown.cpp                                              */
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
 * @file test_period_breakdown.cpp
 * @brief Tests one period's breakdown: group self time, uncovered time and thread totals.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/period_breakdown.hpp>

#include <gtest/gtest.h>

#include <vector>

using SushiProfiler::PeriodBreakdown;
using SushiProfiler::PeriodRecord;
using SushiProfiler::SiteRecord;
using SushiProfiler::ZoneNode;
using SushiProfiler::ZoneTree;

namespace
{
    /// @brief Returns sites 1 `Model`, 2 ungrouped and 3 `Data`; site 0 is empty.
    std::vector<SiteRecord> sites()
    {
        std::vector<SiteRecord> table(4U);
        table[1].group = "Model";
        table[3].group = "Data";
        return table;
    }

    /// @brief Returns a node of @p site under @p parent with @p inclusive and @p self time.
    ZoneNode node(SushiProfiler::SiteID site, std::int32_t parent, std::uint64_t inclusive,
                  std::uint64_t self)
    {
        ZoneNode result;
        result.site = site;
        result.parent = parent;
        result.inclusive_ns = inclusive;
        result.self_ns = self;
        result.calls = 1U;
        return result;
    }

    /// @brief Returns a 100 ns period marked by thread 0.
    PeriodRecord period_of_100()
    {
        PeriodRecord period;
        period.thread = 0U;
        period.begin_ns = 0U;
        period.end_ns = 100U;
        return period;
    }
} // namespace

TEST(Small_PeriodBreakdown, InheritsTheNearestGroupedAncestorsGroup)
{
    PeriodRecord period = period_of_100();
    ZoneTree tree;
    tree.thread = 0U;
    tree.nodes = {node(1U, -1, 60U, 20U), node(2U, 0, 40U, 40U)};
    period.zones.push_back(tree);

    const PeriodBreakdown breakdown = SushiProfiler::break_down_period(period, sites());

    ASSERT_EQ(breakdown.groups.size(), 1U);
    EXPECT_EQ(breakdown.groups[0].group, "Model");
    EXPECT_EQ(breakdown.groups[0].self_ns, 60U);
}

TEST(Small_PeriodBreakdown, ChargesUngroupedZonesToTheUngroupedLabel)
{
    PeriodRecord period = period_of_100();
    ZoneTree tree;
    tree.thread = 0U;
    tree.nodes = {node(2U, -1, 30U, 30U)};
    period.zones.push_back(tree);

    const PeriodBreakdown breakdown = SushiProfiler::break_down_period(period, sites());

    ASSERT_EQ(breakdown.groups.size(), 1U);
    EXPECT_EQ(breakdown.groups[0].group, SushiProfiler::K_UNGROUPED_GROUP);
}

TEST(Small_PeriodBreakdown, OrdersGroupsByDescendingSelfTime)
{
    PeriodRecord period = period_of_100();
    ZoneTree tree;
    tree.thread = 0U;
    tree.nodes = {node(1U, -1, 20U, 20U), node(3U, -1, 50U, 50U)};
    period.zones.push_back(tree);

    const PeriodBreakdown breakdown = SushiProfiler::break_down_period(period, sites());

    ASSERT_EQ(breakdown.groups.size(), 2U);
    EXPECT_EQ(breakdown.groups[0].group, "Data");
    EXPECT_EQ(breakdown.groups[1].group, "Model");
}

TEST(Small_PeriodBreakdown, OrdersGroupsOfEqualSelfTimeByName)
{
    PeriodRecord period = period_of_100();
    ZoneTree tree;
    tree.thread = 0U;
    tree.nodes = {node(1U, -1, 40U, 40U), node(3U, -1, 40U, 40U)};
    period.zones.push_back(tree);

    const PeriodBreakdown breakdown = SushiProfiler::break_down_period(period, sites());

    ASSERT_EQ(breakdown.groups.size(), 2U);
    EXPECT_EQ(breakdown.groups[0].group, "Data");
    EXPECT_EQ(breakdown.groups[1].group, "Model");
}

TEST(Small_PeriodBreakdown, CountsOnlyTheMarkingThreadsRootsAsCovered)
{
    PeriodRecord period = period_of_100();
    ZoneTree marking;
    marking.thread = 0U;
    marking.nodes = {node(1U, -1, 40U, 40U)};
    ZoneTree other;
    other.thread = 1U;
    other.nodes = {node(3U, -1, 90U, 90U)};
    period.zones = {marking, other};

    const PeriodBreakdown breakdown = SushiProfiler::break_down_period(period, sites());

    EXPECT_EQ(breakdown.uncovered_ns, 60U);
    ASSERT_EQ(breakdown.groups.size(), 1U);
    EXPECT_EQ(breakdown.groups[0].group, "Model");
}

TEST(Small_PeriodBreakdown, UncoveredIsZeroWhenZonesOverrunThePeriod)
{
    PeriodRecord period = period_of_100();
    ZoneTree tree;
    tree.thread = 0U;
    tree.nodes = {node(1U, -1, 130U, 130U)};
    period.zones.push_back(tree);

    EXPECT_EQ(SushiProfiler::break_down_period(period, sites()).uncovered_ns, 0U);
}

TEST(Small_PeriodBreakdown, ReportsEveryThreadsTotalsWithLanesSummed)
{
    PeriodRecord period = period_of_100();
    ZoneTree first;
    first.thread = 1U;
    first.lane = 0U;
    first.nodes = {node(1U, -1, 30U, 10U), node(2U, 0, 20U, 20U)};
    ZoneTree second;
    second.thread = 1U;
    second.lane = 5U;
    second.nodes = {node(3U, -1, 15U, 15U)};
    ZoneTree marking;
    marking.thread = 0U;
    marking.nodes = {node(1U, -1, 50U, 50U)};
    period.zones = {marking, first, second};

    const PeriodBreakdown breakdown = SushiProfiler::break_down_period(period, sites());

    ASSERT_EQ(breakdown.threads.size(), 2U);
    EXPECT_EQ(breakdown.threads[0].thread, 0U);
    EXPECT_EQ(breakdown.threads[0].root_inclusive_ns, 50U);
    EXPECT_EQ(breakdown.threads[1].thread, 1U);
    EXPECT_EQ(breakdown.threads[1].root_inclusive_ns, 45U);
    EXPECT_EQ(breakdown.threads[1].self_ns, 45U);
}
