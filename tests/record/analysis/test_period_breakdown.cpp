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
 * @file test_period_breakdown.cpp
 * @brief Tests one period's breakdown: group self time, uncovered time and thread totals.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/period_breakdown.hpp>

#include <SushiProfiler/analysis/breakdown_analysis.hpp>

#include <gtest/gtest.h>

#include <vector>

#include "analysis_record.hpp"

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

TEST(Small_PeriodBreakdown, MatchesTheBreakdownAnalysisOnOnePeriod)
{
    const SushiProfiler::RunRecord record = SushiProfilerTests::analysis_record();
    const PeriodRecord* active = nullptr;
    for (const PeriodRecord& period : record.periods)
    {
        if (!period.warmup)
        {
            active = &period;
            break;
        }
    }
    ASSERT_NE(active, nullptr);
    SushiProfiler::RunRecord single = record;
    single.periods = {*active};

    const SushiProfiler::Table table = SushiProfiler::BreakdownAnalysis().run(single, {});
    const PeriodBreakdown breakdown = SushiProfiler::break_down_period(*active, record.sites);

    ASSERT_EQ(table.rows.size(), breakdown.groups.size() + 1U);
    for (std::size_t row = 0U; row < breakdown.groups.size(); ++row)
    {
        bool found = false;
        for (const SushiProfiler::Row& candidate : table.rows)
        {
            if (candidate[1].as_text() != nullptr && *candidate[1].as_text() == breakdown.groups[row].group)
            {
                ASSERT_NE(candidate[3].as_real(), nullptr);
                EXPECT_DOUBLE_EQ(*candidate[3].as_real(),
                                 static_cast<double>(breakdown.groups[row].self_ns));
                found = true;
            }
        }
        EXPECT_TRUE(found);
    }
}
