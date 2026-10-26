/**************************************************************************/
/* test_period_publisher.cpp                                              */
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
 * @file test_period_publisher.cpp
 * @brief Tests that the publisher publishes only to a watched channel and numbers what it does.
 * @author Mustafa Garip
 * @see tests/session/live/README.md
 */

#include <SushiProfiler/live/period_publisher.hpp>

#include <gtest/gtest.h>

#include <memory>

using SushiProfiler::LiveChannel;
using SushiProfiler::PeriodPublisher;
using SushiProfiler::PeriodRecord;
using SushiProfiler::RunCap;
using SushiProfiler::RunRecord;
using SushiProfiler::ThreadRecord;

namespace
{
    /// @brief Returns a record with one name, one thread and the counts the tests sum.
    RunRecord counted_record()
    {
        RunRecord record;
        record.names = {"", "step"};
        record.sites.resize(1U);
        ThreadRecord first;
        first.index = 0U;
        first.late = 2U;
        first.dropped = 5U;
        ThreadRecord second;
        second.index = 1U;
        second.late = 3U;
        second.dropped = 7U;
        record.threads = {first, second};
        record.drops.spans_dropped = 11U;
        record.drops.outside_periods = 13U;
        return record;
    }
} // namespace

TEST(Small_PeriodPublisher, PublishesNothingWhileTheChannelIsNotWatched)
{
    LiveChannel channel(4U);
    PeriodPublisher publisher(channel, RunCap{10U, 0U}, 1000U);

    publisher.closed(PeriodRecord{}, counted_record());

    EXPECT_EQ(channel.published(), 0U);
    EXPECT_EQ(channel.latest(), nullptr);
}

TEST(Small_PeriodPublisher, PublishesWithConsecutiveSequencesAndTheChannelsRun)
{
    LiveChannel channel(4U);
    static_cast<void>(channel.begin_run());
    channel.attach();
    PeriodPublisher publisher(channel, RunCap{10U, 0U}, 1000U);
    const RunRecord record = counted_record();
    PeriodRecord period;
    period.index = 40U;

    publisher.closed(period, record);
    publisher.closed(period, record);

    const auto history = channel.history(10U);
    ASSERT_EQ(history.size(), 2U);
    EXPECT_EQ(history[0]->sequence, 0U);
    EXPECT_EQ(history[1]->sequence, 1U);
    EXPECT_EQ(history[1]->run, 1U);
    EXPECT_EQ(history[1]->origin_steady_ns, 1000U);
    EXPECT_EQ(history[1]->period.index, 40U);
}

TEST(Small_PeriodPublisher, SharesNamesUntilATableGrows)
{
    LiveChannel channel(4U);
    channel.attach();
    PeriodPublisher publisher(channel, RunCap{10U, 0U}, 0U);
    RunRecord record = counted_record();

    publisher.closed(PeriodRecord{}, record);
    publisher.closed(PeriodRecord{}, record);
    record.names.push_back("conv");
    publisher.closed(PeriodRecord{}, record);

    const auto history = channel.history(10U);
    ASSERT_EQ(history.size(), 3U);
    EXPECT_EQ(history[0]->names, history[1]->names);
    EXPECT_NE(history[1]->names, history[2]->names);
    EXPECT_EQ(history[2]->names->name(2U), "conv");
    EXPECT_EQ(history[0]->names->name(2U), "");
}

TEST(Small_PeriodPublisher, SumsTheRecordsLateAndDroppedCounts)
{
    LiveChannel channel(4U);
    channel.attach();
    PeriodPublisher publisher(channel, RunCap{10U, 0U}, 0U);

    publisher.closed(PeriodRecord{}, counted_record());

    const auto latest = channel.latest();
    ASSERT_NE(latest, nullptr);
    EXPECT_EQ(latest->counts.late, 5U);
    EXPECT_EQ(latest->counts.dropped, 12U);
    EXPECT_EQ(latest->counts.spans_dropped, 11U);
    EXPECT_EQ(latest->counts.outside, 13U);
}
