/**************************************************************************/
/* test_period_fold.cpp                                                   */
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
 * @file test_period_fold.cpp
 * @brief Tests zone trees, self time, lanes, threads, samples and unmatched zones of a fold.
 * @author Mustafa Garip
 * @see tests/session/fold/README.md
 */

#include <SushiProfiler/fold/period_fold.hpp>

#include <gtest/gtest.h>

using SushiProfiler::EventKind;
using SushiProfiler::FlowPoint;
using SushiProfiler::FoldEvent;
using SushiProfiler::NameID;
using SushiProfiler::PeriodFold;
using SushiProfiler::SampleSeries;
using SushiProfiler::SiteID;
using SushiProfiler::ThreadEvents;
using SushiProfiler::ZoneInstance;
using SushiProfiler::ZoneNode;
using SushiProfiler::ZoneTree;

namespace
{
    /// @brief Returns a zone event of @p site from @p begin to @p end in @p lane.
    FoldEvent zone(SiteID site, std::uint64_t begin, std::uint64_t end, NameID lane = 0U)
    {
        return FoldEvent{{begin, end, SushiProfiler::pack_site(EventKind::Zone, site), 0U}, lane};
    }

    /// @brief Returns an event of @p kind and @p site at @p time whose second field is @p end.
    FoldEvent point(EventKind kind, SiteID site, std::uint64_t time, std::uint64_t end)
    {
        return FoldEvent{{time, end, SushiProfiler::pack_site(kind, site), 0U}, 0U};
    }

    /// @brief Returns a node of @p site under @p parent with its times and calls.
    ZoneNode node(SiteID site, std::int32_t parent, std::uint64_t inclusive, std::uint64_t self,
                  std::uint64_t calls)
    {
        ZoneNode result;
        result.site = site;
        result.parent = parent;
        result.inclusive_ns = inclusive;
        result.self_ns = self;
        result.calls = calls;
        return result;
    }

    /// @brief Checks that @p actual equals @p expected, field by field.
    void expect_nodes(const std::vector<ZoneNode>& actual, const std::vector<ZoneNode>& expected)
    {
        ASSERT_EQ(actual.size(), expected.size());
        for (std::size_t index = 0U; index < actual.size(); ++index)
        {
            EXPECT_EQ(actual[index].site, expected[index].site) << "node " << index;
            EXPECT_EQ(actual[index].name, expected[index].name) << "node " << index;
            EXPECT_EQ(actual[index].parent, expected[index].parent) << "node " << index;
            EXPECT_EQ(actual[index].inclusive_ns, expected[index].inclusive_ns) << "node " << index;
            EXPECT_EQ(actual[index].self_ns, expected[index].self_ns) << "node " << index;
            EXPECT_EQ(actual[index].calls, expected[index].calls) << "node " << index;
        }
    }
} // namespace

TEST(Small_PeriodFold, NestsZonesByTimeAndMergesCalls)
{
    const std::vector<ThreadEvents> threads = {
        {0U, {zone(2U, 10U, 20U), zone(2U, 30U, 40U), zone(3U, 50U, 60U), zone(1U, 0U, 100U)}}};

    const PeriodFold fold = SushiProfiler::fold_period(threads);

    ASSERT_EQ(fold.zones.size(), 1U);
    expect_nodes(fold.zones[0].nodes,
                 {node(1U, -1, 100U, 70U, 1U), node(2U, 0, 20U, 20U, 2U),
                  node(3U, 0, 10U, 10U, 1U)});
    EXPECT_TRUE(fold.unmatched.empty());
}

TEST(Small_PeriodFold, OneSiteUnderTwoParentsIsTwoNodes)
{
    const std::vector<ThreadEvents> threads = {
        {0U, {zone(3U, 5U, 8U), zone(1U, 0U, 10U), zone(3U, 25U, 26U), zone(2U, 20U, 30U)}}};

    const PeriodFold fold = SushiProfiler::fold_period(threads);

    expect_nodes(fold.zones[0].nodes, {node(1U, -1, 10U, 7U, 1U), node(3U, 0, 3U, 3U, 1U),
                                       node(2U, -1, 10U, 9U, 1U), node(3U, 2, 1U, 1U, 1U)});
}

TEST(Small_PeriodFold, EqualRunsNestInRingOrder)
{
    const std::vector<ThreadEvents> threads = {{0U, {zone(2U, 10U, 20U), zone(1U, 10U, 20U)}}};

    const PeriodFold fold = SushiProfiler::fold_period(threads);

    expect_nodes(fold.zones[0].nodes, {node(1U, -1, 10U, 0U, 1U), node(2U, 0, 10U, 10U, 1U)});
}

TEST(Small_PeriodFold, SplitsTreesByThreadThenLane)
{
    const std::vector<ThreadEvents> threads = {
        {4U, {zone(1U, 0U, 10U, 7U), zone(1U, 20U, 25U)}},
        {2U, {zone(2U, 0U, 3U)}}};

    const PeriodFold fold = SushiProfiler::fold_period(threads);

    ASSERT_EQ(fold.zones.size(), 3U);
    EXPECT_EQ(std::make_pair(fold.zones[0].thread, fold.zones[0].lane), std::make_pair(2U, 0U));
    EXPECT_EQ(std::make_pair(fold.zones[1].thread, fold.zones[1].lane), std::make_pair(4U, 0U));
    EXPECT_EQ(std::make_pair(fold.zones[2].thread, fold.zones[2].lane), std::make_pair(4U, 7U));
    EXPECT_EQ(fold.zones[2].nodes[0].inclusive_ns, 10U);
}

TEST(Small_PeriodFold, KeepsEveryRunWithItsDepth)
{
    const std::vector<ThreadEvents> threads = {{0U, {zone(2U, 10U, 20U), zone(1U, 0U, 100U)}}};

    const PeriodFold fold = SushiProfiler::fold_period(threads);

    ASSERT_EQ(fold.instances.size(), 2U);
    EXPECT_EQ(fold.instances[0].site, 1U);
    EXPECT_EQ(fold.instances[0].depth, 0U);
    EXPECT_EQ(fold.instances[1].site, 2U);
    EXPECT_EQ(fold.instances[1].begin_ns, 10U);
    EXPECT_EQ(fold.instances[1].depth, 1U);
}

TEST(Small_PeriodFold, CountsZonesThatDoNotNest)
{
    const std::vector<ThreadEvents> threads = {
        {3U, {zone(1U, 0U, 50U), zone(2U, 40U, 80U), zone(4U, 90U, 85U)}}};

    const PeriodFold fold = SushiProfiler::fold_period(threads);

    expect_nodes(fold.zones[0].nodes, {node(1U, -1, 50U, 50U, 1U), node(2U, -1, 40U, 40U, 1U)});
    ASSERT_EQ(fold.unmatched.size(), 1U);
    EXPECT_EQ(fold.unmatched[0].thread, 3U);
    EXPECT_EQ(fold.unmatched[0].count, 2U);
}

TEST(Small_PeriodFold, FoldsSamplesBySiteThenThread)
{
    const std::vector<ThreadEvents> threads = {
        {1U,
         {point(EventKind::Counter, 5U, 10U, SushiProfiler::value_bits(3.0)),
          point(EventKind::Instant, 6U, 12U, 12U),
          point(EventKind::FlowBegin, 7U, 13U, 99U),
          point(EventKind::PeriodMark, 0U, 14U, 2U),
          point(EventKind::LaneBegin, 0U, 15U, 0U),
          point(EventKind::Counter, 5U, 20U, SushiProfiler::value_bits(4.5))}},
        {0U,
         {point(EventKind::Value, 5U, 11U, SushiProfiler::value_bits(-1.0)),
          point(EventKind::FlowEnd, 7U, 30U, 99U)}}};

    const PeriodFold fold = SushiProfiler::fold_period(threads);

    ASSERT_EQ(fold.samples.size(), 5U);
    const SampleSeries& counter = fold.samples[1];
    EXPECT_EQ(std::make_pair(counter.site, counter.thread), std::make_pair(5U, 1U));
    EXPECT_EQ(counter.time_ns, (std::vector<std::uint64_t>{10U, 20U}));
    EXPECT_EQ(counter.values, (std::vector<double>{3.0, 4.5}));
    EXPECT_EQ(fold.samples[0].values, std::vector<double>{-1.0});
    EXPECT_EQ(fold.samples[2].site, 6U);
    EXPECT_TRUE(fold.samples[2].values.empty());
    const SampleSeries& flow_end = fold.samples[3];
    EXPECT_EQ(std::make_pair(flow_end.site, flow_end.thread), std::make_pair(7U, 0U));
    EXPECT_EQ(flow_end.points, std::vector<FlowPoint>{FlowPoint::End});
    EXPECT_EQ(fold.samples[4].items, std::vector<std::uint64_t>{99U});
    EXPECT_TRUE(fold.zones.empty());
}

TEST(Small_PeriodFold, TheSameEventsFoldTheSame)
{
    const std::vector<ThreadEvents> threads = {
        {0U, {zone(2U, 10U, 20U), zone(1U, 0U, 100U),
              point(EventKind::Value, 3U, 5U, SushiProfiler::value_bits(1.0))}}};

    const PeriodFold first = SushiProfiler::fold_period(threads);
    const PeriodFold second = SushiProfiler::fold_period(threads);

    ASSERT_EQ(first.zones.size(), second.zones.size());
    EXPECT_TRUE(first.zones[0] == second.zones[0]);
    EXPECT_TRUE(first.samples[0] == second.samples[0]);
}
