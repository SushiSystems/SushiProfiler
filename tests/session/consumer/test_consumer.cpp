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
 * @file test_consumer.cpp
 * @brief Tests the adaptive poll, the consumer's pacing on a manual clock and a real drain.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/consumer.hpp>

#include <SushiProfiler/clock/manual_tick_source.hpp>
#include <SushiProfiler/consumer/manual_poll_clock.hpp>

#include <gtest/gtest.h>

#include <SushiProfiler/zones/macros.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "assembly_rig.hpp"

using SushiProfiler::AssemblySettings;
using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::Consumer;
using SushiProfiler::DrainBatch;
using SushiProfiler::ManualPollClock;
using SushiProfiler::ManualTickSource;
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

namespace
{
    /// @brief Records three calls of the zone `consumer.work` on the calling thread.
    void work_three_times()
    {
        for (int call = 0; call < 3; ++call)
        {
            SP_ZONE("consumer.work");
        }
    }

    /// @brief Returns the calls of the zone named @p name in period @p index of @p record.
    std::uint64_t calls_of(const RunRecord& record, std::size_t index, const std::string& name)
    {
        std::uint64_t calls = 0U;
        for (const auto& tree : record.periods[index].zones)
        {
            for (const ZoneNode& node : tree.nodes)
                calls += record.sites[node.site].name == name ? node.calls : 0U;
        }
        return calls;
    }
} // namespace

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
    work_three_times();
    collector.close();
    consumer.stop();
    consumer.stop();

    ASSERT_EQ(record.periods.size(), 1U);
    EXPECT_EQ(calls_of(record, 0U, "consumer.work"), 3U);
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
    /// @brief Builds a collector on a manual tick source, with the record and assembler a live
    ///        consumer test needs, and a manual poll clock to pace the consumer with.
    class LiveConsumerRig
    {
        public:
            LiveConsumerRig()
            {
                opened = collector.open(CollectorSettings{});
                settings.origin = collector.first_point();
                settings.run_timeline = collector.names().intern("run");
                settings.linger_ns = 0U;
                assembler = std::make_unique<PeriodAssembler>(
                    settings, record, collector.names(), collector.sites(),
                    std::vector<SushiProfiler::Source*>{}, rig.parts());
            }

            /// @brief Lets the consumer through @p polls polls, each ending when its drain is done.
            void poll_through(std::size_t polls)
            {
                for (std::size_t poll = 0U; poll < polls; ++poll)
                    clock.poll_once();
            }

            ManualTickSource ticks{100U};                  /**< The collector's stamps. */
            Collector collector{ticks};                    /**< The collector drained. */
            bool opened = false;                           /**< Whether the collector opened. */
            RunRecord record;                              /**< The record filled. */
            AssemblySettings settings;                     /**< The assembler's settings. */
            SushiProfilerTests::AssemblyRig rig;           /**< The assembler's collaborators. */
            std::unique_ptr<PeriodAssembler> assembler;    /**< The assembler. */
            ManualPollClock clock;                         /**< The consumer's pacing. */
    };
} // namespace

TEST(Small_Consumer, AWatchedConsumerDrainsAtTheLiveCeiling)
{
    LiveConsumerRig rig;
    ASSERT_TRUE(rig.opened);
    rig.rig.channel().attach();
    Consumer consumer(rig.collector, *rig.assembler, rig.rig.channel(), milliseconds(4),
                      rig.clock);

    consumer.start();
    rig.poll_through(25U);
    const std::uint64_t drains = consumer.drains();
    rig.collector.close();
    consumer.stop();
    rig.rig.channel().detach();

    const std::vector<milliseconds> limits = rig.clock.limits();
    EXPECT_EQ(drains, 25U);
    EXPECT_EQ(*std::max_element(limits.begin(), limits.end()), milliseconds(4));
    EXPECT_EQ(limits.back(), milliseconds(4));
}

TEST(Small_Consumer, AnUnwatchedConsumerDoublesItsWaitUpToTheSlowestPoll)
{
    LiveConsumerRig rig;
    ASSERT_TRUE(rig.opened);
    Consumer consumer(rig.collector, *rig.assembler, rig.rig.channel(), milliseconds(4),
                      rig.clock);

    consumer.start();
    rig.poll_through(12U);
    rig.collector.close();
    consumer.stop();

    const std::vector<milliseconds> expected{
        milliseconds(1),   milliseconds(2),   milliseconds(4),   milliseconds(8),
        milliseconds(16),  milliseconds(32),  milliseconds(64),  milliseconds(128),
        milliseconds(256), milliseconds(512), milliseconds(1000), milliseconds(1000),
        milliseconds(1000)};
    EXPECT_EQ(rig.clock.limits(), expected);
}

TEST(Small_Consumer, WakeShortensAnIdleSleep)
{
    LiveConsumerRig rig;
    ASSERT_TRUE(rig.opened);
    Consumer consumer(rig.collector, *rig.assembler, rig.rig.channel(), milliseconds(4),
                      rig.clock);
    consumer.start();
    rig.poll_through(10U);
    const std::uint64_t before = consumer.drains();

    rig.rig.channel().attach();
    consumer.wake();
    rig.clock.await_waits(12U);
    const std::uint64_t after = consumer.drains();
    const std::vector<milliseconds> limits = rig.clock.limits();
    rig.collector.close();
    consumer.stop();
    rig.rig.channel().detach();

    EXPECT_EQ(limits[10], milliseconds(1000));
    EXPECT_EQ(after, before + 1U);
    EXPECT_EQ(limits[11], milliseconds(4));
}
