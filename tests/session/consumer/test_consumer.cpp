/**************************************************************************/
/* test_consumer.cpp                                                      */
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
 * @file test_consumer.cpp
 * @brief Tests the adaptive poll and a consumer draining a real collector into a record.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/consumer.hpp>

#include <gtest/gtest.h>

#include <SushiProfiler/zones/macros.hpp>

using SushiProfiler::AssemblySettings;
using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::Consumer;
using SushiProfiler::DrainBatch;
using SushiProfiler::PeriodAssembler;
using SushiProfiler::RunRecord;
using SushiProfiler::ZoneNode;
using std::chrono::milliseconds;

TEST(Small_NextPoll, SpeedsUpWhenFullAndSlowsDownWhenQuiet)
{
    EXPECT_EQ(SushiProfiler::next_poll(milliseconds(400), 0.6), milliseconds(1));
    EXPECT_EQ(SushiProfiler::next_poll(milliseconds(400), 0.3), milliseconds(400));
    EXPECT_EQ(SushiProfiler::next_poll(milliseconds(400), 0.1), milliseconds(800));
    EXPECT_EQ(SushiProfiler::next_poll(milliseconds(800), 0.0), milliseconds(1000));
    EXPECT_EQ(SushiProfiler::next_poll(milliseconds(1000), 0.0), milliseconds(1000));
}

TEST(Medium_Consumer, DrainsWhileRunningAndFinishesOnStop)
{
    Collector& collector = Collector::instance();
    collector.close();
    DrainBatch leftover;
    collector.drain(leftover);
    ASSERT_TRUE(collector.open(CollectorSettings{}));
    RunRecord record;
    AssemblySettings settings;
    settings.origin = collector.first_point();
    settings.run_timeline = collector.names().intern("run");
    settings.linger_ns = 0U;
    PeriodAssembler assembler(settings, record, collector.names(), collector.sites(), {}, {});
    Consumer consumer(collector, assembler);

    consumer.start();
    for (int call = 0; call < 3; ++call)
    {
        SP_ZONE("consumer.work");
    }
    collector.close();
    consumer.stop();
    consumer.stop();

    ASSERT_EQ(record.periods.size(), 1U);
    std::uint64_t calls = 0U;
    for (const auto& tree : record.periods[0].zones)
    {
        for (const ZoneNode& node : tree.nodes)
        {
            if (record.sites[node.site].name == "consumer.work")
                calls += node.calls;
        }
    }
    EXPECT_EQ(calls, 3U);
    EXPECT_FALSE(record.threads.empty());
    EXPECT_GE(consumer.drains(), 1U);
}
