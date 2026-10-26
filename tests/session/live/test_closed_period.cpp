/**************************************************************************/
/* test_closed_period.cpp                                                 */
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
 * @file test_closed_period.cpp
 * @brief Tests that a closed period copies the period and caps its zone runs as stated.
 * @author Mustafa Garip
 * @see tests/session/live/README.md
 */

#include <SushiProfiler/live/closed_period.hpp>

#include <gtest/gtest.h>

#include <tuple>
#include <vector>

using SushiProfiler::ClosedPeriod;
using SushiProfiler::LiveCounts;
using SushiProfiler::PeriodRecord;
using SushiProfiler::RunCap;
using SushiProfiler::ZoneRun;

namespace
{
    /// @brief Returns a run of @p duration ns on @p thread starting at @p begin.
    ZoneRun run_of(std::uint32_t thread, std::uint64_t begin, std::uint64_t duration)
    {
        ZoneRun run;
        run.thread = thread;
        run.site = 1U;
        run.begin_ns = begin;
        run.end_ns = begin + duration;
        return run;
    }

    /// @brief Returns a period whose runs last 10, 50, 30, 40 and 20 ns, in begin order.
    PeriodRecord period_with_runs()
    {
        PeriodRecord period;
        period.index = 4U;
        period.begin_ns = 0U;
        period.end_ns = 1000U;
        period.runs = {run_of(0U, 0U, 10U), run_of(0U, 100U, 50U), run_of(0U, 200U, 30U),
                       run_of(0U, 300U, 40U), run_of(0U, 400U, 20U)};
        return period;
    }

    /// @brief Returns the durations of @p closed's runs.
    std::vector<std::uint64_t> durations(const ClosedPeriod& closed)
    {
        std::vector<std::uint64_t> out;
        for (const ZoneRun& run : closed.period.runs)
            out.push_back(run.end_ns - run.begin_ns);
        return out;
    }
} // namespace

TEST(Small_ClosedPeriod, KeepsTheLongestRunsAndCountsTheOmitted)
{
    const auto closed = SushiProfiler::make_closed_period(period_with_runs(), 1U, 0U, 0U,
                                                          RunCap{2U, 0U}, LiveCounts{}, nullptr);

    EXPECT_EQ(durations(*closed), (std::vector<std::uint64_t>{50U, 40U}));
    EXPECT_EQ(closed->runs_omitted, 3U);
}

TEST(Small_ClosedPeriod, DropsRunsBelowTheMinimumLength)
{
    const auto closed = SushiProfiler::make_closed_period(
        period_with_runs(), 1U, 0U, 0U, RunCap{100U, 25U}, LiveCounts{}, nullptr);

    EXPECT_EQ(durations(*closed), (std::vector<std::uint64_t>{50U, 30U, 40U}));
    EXPECT_EQ(closed->runs_omitted, 2U);
}

TEST(Small_ClosedPeriod, KeepsRunsInThreadLaneBeginOrder)
{
    PeriodRecord period;
    period.runs = {run_of(0U, 0U, 5U),  run_of(0U, 10U, 90U), run_of(1U, 0U, 80U),
                   run_of(1U, 50U, 70U), run_of(2U, 5U, 60U), run_of(2U, 30U, 3U)};

    const auto closed = SushiProfiler::make_closed_period(period, 1U, 0U, 0U, RunCap{4U, 0U},
                                                          LiveCounts{}, nullptr);

    ASSERT_EQ(closed->period.runs.size(), 4U);
    for (std::size_t index = 1U; index < closed->period.runs.size(); ++index)
    {
        const ZoneRun& before = closed->period.runs[index - 1U];
        const ZoneRun& after = closed->period.runs[index];
        EXPECT_LT(std::tie(before.thread, before.lane, before.begin_ns),
                  std::tie(after.thread, after.lane, after.begin_ns));
    }
}

TEST(Small_ClosedPeriod, ZeroMaxRunsKeepsNone)
{
    const auto closed = SushiProfiler::make_closed_period(period_with_runs(), 1U, 0U, 0U,
                                                          RunCap{0U, 0U}, LiveCounts{}, nullptr);

    EXPECT_TRUE(closed->period.runs.empty());
    EXPECT_EQ(closed->runs_omitted, 5U);
}

TEST(Small_ClosedPeriod, AWarmupPeriodHasNoRuns)
{
    PeriodRecord period;
    period.warmup = true;

    const auto closed = SushiProfiler::make_closed_period(period, 1U, 0U, 0U, RunCap{10U, 0U},
                                                          LiveCounts{}, nullptr);

    EXPECT_TRUE(closed->period.runs.empty());
    EXPECT_EQ(closed->runs_omitted, 0U);
    EXPECT_TRUE(closed->period.warmup);
}

TEST(Small_ClosedPeriod, LeavesTheSourcePeriodUntouched)
{
    const PeriodRecord period = period_with_runs();

    const auto closed = SushiProfiler::make_closed_period(period, 1U, 0U, 0U, RunCap{1U, 0U},
                                                          LiveCounts{}, nullptr);

    EXPECT_EQ(period.runs.size(), 5U);
    EXPECT_EQ(closed->period.runs.size(), 1U);
}

TEST(Small_ClosedPeriod, CarriesItsRunSequenceOriginAndCounts)
{
    LiveCounts counts;
    counts.late = 1U;
    counts.dropped = 2U;
    counts.spans_dropped = 3U;
    counts.outside = 4U;

    const auto closed = SushiProfiler::make_closed_period(PeriodRecord{}, 3U, 7U, 123U, RunCap{},
                                                          counts, nullptr);

    EXPECT_EQ(closed->run, 3U);
    EXPECT_EQ(closed->sequence, 7U);
    EXPECT_EQ(closed->origin_steady_ns, 123U);
    EXPECT_EQ(closed->counts.late, 1U);
    EXPECT_EQ(closed->counts.dropped, 2U);
    EXPECT_EQ(closed->counts.spans_dropped, 3U);
    EXPECT_EQ(closed->counts.outside, 4U);
}

TEST(Small_ClosedPeriod, CopiesEveryFieldButTheRuns)
{
    PeriodRecord period = period_with_runs();
    period.timeline = 2U;
    period.thread = 1U;
    period.warmup = false;
    period.budget_ns = 16'666'667U;
    period.tags.push_back(SushiProfiler::Tag{1U, 2U});
    period.facts.push_back(SushiProfiler::Fact{"paused", "true"});
    SushiProfiler::ZoneTree tree;
    tree.thread = 1U;
    tree.nodes.push_back(SushiProfiler::ZoneNode{1U, 0U, -1, 100U, 60U, 2U});
    period.zones.push_back(tree);
    SushiProfiler::Span span;
    span.start_ns = 5U;
    span.queued_ns = 5U;
    span.end_ns = 9U;
    static_cast<void>(period.spans.append(span));
    SushiProfiler::SampleSeries series;
    series.site = 3U;
    series.time_ns = {1U, 2U};
    series.values = {0.5, 1.5};
    period.samples.push_back(series);

    const auto closed = SushiProfiler::make_closed_period(period, 1U, 0U, 0U, RunCap{100U, 0U},
                                                          LiveCounts{}, nullptr);

    EXPECT_EQ(closed->period, period);
}
