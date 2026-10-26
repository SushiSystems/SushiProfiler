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

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "assembly_rig.hpp"

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
    SushiProfilerTests::AssemblyRig rig;
    PeriodAssembler assembler(settings, record, collector.names(), collector.sites(), {},
                              rig.parts());
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

TEST(Small_NextPoll, CeilingLimitsTheDoubling)
{
    EXPECT_EQ(SushiProfiler::next_poll(milliseconds(1), 0.0, milliseconds(4)), milliseconds(2));
    EXPECT_EQ(SushiProfiler::next_poll(milliseconds(3), 0.0, milliseconds(4)), milliseconds(4));
    EXPECT_EQ(SushiProfiler::next_poll(milliseconds(400), 0.3, milliseconds(4)), milliseconds(4));
    EXPECT_EQ(SushiProfiler::next_poll(milliseconds(400), 0.0, milliseconds(4)), milliseconds(4));
}

TEST(Small_NextPoll, TheFastestPollStillWinsWhenARingIsFull)
{
    EXPECT_EQ(SushiProfiler::next_poll(milliseconds(4), 0.9, milliseconds(4)), milliseconds(1));
}

namespace
{
    /// @brief Opens the collector and builds the record and assembler a live consumer test needs.
    class LiveConsumerRig
    {
        public:
            LiveConsumerRig()
            {
                Collector& collector = Collector::instance();
                collector.close();
                DrainBatch leftover;
                collector.drain(leftover);
                opened = collector.open(CollectorSettings{});
                settings.origin = collector.first_point();
                settings.run_timeline = collector.names().intern("run");
                settings.linger_ns = 0U;
                assembler = std::make_unique<PeriodAssembler>(
                    settings, record, collector.names(), collector.sites(),
                    std::vector<SushiProfiler::Source*>{}, rig.parts());
            }

            bool opened = false;                           /**< Whether the collector opened. */
            RunRecord record;                              /**< The record filled. */
            AssemblySettings settings;                     /**< The assembler's settings. */
            SushiProfilerTests::AssemblyRig rig;           /**< The assembler's collaborators. */
            std::unique_ptr<PeriodAssembler> assembler;    /**< The assembler. */
    };

    /// @brief Waits up to @p limit for @p condition; returns whether it held.
    template <class Condition>
    bool wait_for_condition(milliseconds limit, Condition condition)
    {
        const auto end = std::chrono::steady_clock::now() + limit;
        while (std::chrono::steady_clock::now() < end)
        {
            if (condition())
                return true;
            std::this_thread::sleep_for(milliseconds(1));
        }
        return condition();
    }
} // namespace

TEST(Medium_Consumer, AWatchedConsumerDrainsAtTheLiveCeiling)
{
    LiveConsumerRig rig;
    ASSERT_TRUE(rig.opened);
    rig.rig.channel().attach();
    Consumer consumer(Collector::instance(), *rig.assembler, rig.rig.channel(), milliseconds(4));

    consumer.start();
    const bool drained =
        wait_for_condition(milliseconds(5'000), [&] { return consumer.drains() >= 25U; });
    Collector::instance().close();
    consumer.stop();
    rig.rig.channel().detach();

    EXPECT_TRUE(drained);
}

TEST(Medium_Consumer, WakeShortensAnIdleSleep)
{
    LiveConsumerRig rig;
    ASSERT_TRUE(rig.opened);
    Consumer consumer(Collector::instance(), *rig.assembler, rig.rig.channel(), milliseconds(4));
    consumer.start();
    std::this_thread::sleep_for(milliseconds(2'500));
    const std::uint64_t before = consumer.drains();

    rig.rig.channel().attach();
    consumer.wake();
    const bool served =
        wait_for_condition(milliseconds(400), [&] { return consumer.drains() > before; });
    Collector::instance().close();
    consumer.stop();
    rig.rig.channel().detach();

    EXPECT_TRUE(served);
}
