/**************************************************************************/
/* test_period_timeline.cpp                                               */
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
 * @file test_period_timeline.cpp
 * @brief Tests that marks number periods from 0, carry their index and budget, and tags attach.
 * @author Mustafa Garip
 * @see tests/capture/periods/README.md
 */

#include <SushiProfiler/periods/period_timeline.hpp>

#include <SushiProfiler/clock/manual_tick_source.hpp>
#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/zones/zone.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <thread>
#include <utility>
#include <vector>

using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::DrainBatch;
using SushiProfiler::EventKind;
using SushiProfiler::ManualTickSource;
using SushiProfiler::PeriodTimeline;
using SushiProfiler::ThreadDrain;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief The zone site the held-gate test writes a zone at.
    constexpr SushiProfiler::Site K_ZONE_SITE{"held.zone", "Test", "test_period_timeline.cpp",
                                              1U, 1U, SushiProfiler::SiteRole::Zone,
                                              SushiProfiler::CounterOp::Add, nullptr};

    /// @brief Gives each test its own open collector on a manual clock that starts at 40 ns.
    class Small_PeriodTimeline : public ::testing::Test
    {
        protected:
            void SetUp() override
            {
                ASSERT_TRUE(collector.open(CollectorSettings{}));
            }

            /// @brief Returns what the calling thread wrote since the last drain.
            std::vector<TraceEvent> drain_mine()
            {
                DrainBatch batch;
                collector.drain(batch);
                for (ThreadDrain& thread : batch.threads)
                {
                    if (thread.thread_index == collector.index_of_thread())
                        return std::move(thread.events);
                }
                return {};
            }

            ManualTickSource clock{40U};
            Collector collector{clock};
    };

    /// @brief The test that starts a thread reads the process-wide thread index, so it uses the
    ///        process-wide collector, opened and emptied around the test.
    class Medium_PeriodTimeline : public ::testing::Test
    {
        protected:
            void SetUp() override
            {
                close_and_empty();
                ASSERT_TRUE(Collector::instance().open(CollectorSettings{}));
            }

            void TearDown() override
            {
                close_and_empty();
            }

        private:
            /// @brief Closes the collector and discards what its rings hold.
            static void close_and_empty()
            {
                Collector::instance().close();
                DrainBatch discarded;
                Collector::instance().drain(discarded);
            }
    };
} // namespace

TEST_F(Small_PeriodTimeline, NoPeriodIsOpenBeforeTheFirstMark)
{
    PeriodTimeline timeline(collector, "train.step");

    EXPECT_EQ(timeline.current(), PeriodTimeline::K_NO_PERIOD);
}

TEST_F(Small_PeriodTimeline, MarksCountFromZero)
{
    PeriodTimeline timeline(collector, "train.step");

    EXPECT_EQ(timeline.mark(), 0U);
    EXPECT_EQ(timeline.mark(), 1U);
    EXPECT_EQ(timeline.current(), 1U);
}

TEST_F(Small_PeriodTimeline, TheTimelineKeepsItsName)
{
    PeriodTimeline timeline(collector, "train.step");

    EXPECT_EQ(timeline.name(), "train.step");
}

TEST_F(Small_PeriodTimeline, TheTimelineInternsItsNameInItsCollector)
{
    PeriodTimeline timeline(collector, "train.step");

    EXPECT_EQ(collector.names().text(timeline.id()), "train.step");
}

TEST_F(Small_PeriodTimeline, AMarkCarriesItsIndex)
{
    PeriodTimeline timeline(collector, "engine.frame");
    timeline.mark();
    timeline.mark();
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(SushiProfiler::kind_of(events[1]), EventKind::PeriodMark);
    EXPECT_EQ(events[1].end, 1U);
}

TEST_F(Small_PeriodTimeline, AMarkCarriesItsTimeline)
{
    PeriodTimeline timeline(collector, "engine.frame");
    timeline.mark();
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, timeline.id());
}

TEST_F(Small_PeriodTimeline, AMarkIsStampedByTheClockOfItsCollector)
{
    PeriodTimeline timeline(collector, "step");
    timeline.mark();
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].begin, 40U);
}

TEST_F(Small_PeriodTimeline, AMarkCarriesItsBudget)
{
    PeriodTimeline timeline(collector, "audio.block", 2'666'667U);
    timeline.mark();
    timeline.mark(1'000'000U);
    timeline.mark(0U);
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 5U);
    EXPECT_EQ(SushiProfiler::kind_of(events[1]), EventKind::PeriodBudget);
    EXPECT_EQ(events[1].end, 2'666'667U);
    EXPECT_EQ(SushiProfiler::kind_of(events[3]), EventKind::PeriodBudget);
    EXPECT_EQ(events[3].end, 1'000'000U);
    EXPECT_EQ(SushiProfiler::kind_of(events[4]), EventKind::PeriodMark);
}

TEST_F(Small_PeriodTimeline, TheIndexAdvancesWhileTheCollectorIsClosed)
{
    PeriodTimeline timeline(collector, "bench.iteration");
    collector.close();
    timeline.mark();
    timeline.mark();
    DrainBatch discarded;
    collector.drain(discarded);
    ASSERT_TRUE(collector.open(CollectorSettings{}));

    EXPECT_EQ(timeline.mark(), 2U);
    const std::vector<TraceEvent> events = drain_mine();
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].end, 2U);
}

TEST_F(Small_PeriodTimeline, ATagCarriesKeyAndValue)
{
    PeriodTimeline timeline(collector, "train.step");
    timeline.mark();
    timeline.tag("variant", "fused");
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(SushiProfiler::kind_of(events[1]), EventKind::PeriodTag);
    EXPECT_EQ(events[1].name, timeline.id());
    const auto key = static_cast<SushiProfiler::NameID>(events[1].end >> 32U);
    const auto value = static_cast<SushiProfiler::NameID>(events[1].end & 0xFFFFFFFFU);
    EXPECT_EQ(collector.names().text(key), "variant");
    EXPECT_EQ(collector.names().text(value), "fused");
}

TEST_F(Small_PeriodTimeline, TwoTimelinesKeepTheirOwnIndex)
{
    PeriodTimeline steps(collector, "train.step");
    PeriodTimeline batches(collector, "data.batch");
    steps.mark();
    batches.mark();
    batches.mark();

    EXPECT_EQ(steps.current(), 0U);
    EXPECT_EQ(batches.current(), 1U);
}

TEST_F(Small_PeriodTimeline, AMarkWrittenWhileHeldReachesTheDrain)
{
    PeriodTimeline timeline(collector, "held.step");
    SushiProfiler::SiteHandle handle{&K_ZONE_SITE};
    collector.hold();
    timeline.mark();
    {
        const SushiProfiler::ScopedZone<true> zone(collector, handle);
    }
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::PeriodMark);
}

TEST_F(Small_PeriodTimeline, ATagWhileHeldIsDropped)
{
    PeriodTimeline timeline(collector, "held.step");
    timeline.mark();
    collector.hold();
    timeline.tag("variant", "fused");
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::PeriodMark);
}

TEST_F(Medium_PeriodTimeline, AMarkFromAnUnregisteredThreadWhileHeldRegistersItsRing)
{
    PeriodTimeline timeline("held.step");
    Collector::instance().hold();
    std::uint32_t marker = Collector::K_NO_THREAD;
    std::thread([&] {
        timeline.mark();
        marker = Collector::thread_index();
    }).join();
    DrainBatch batch;
    Collector::instance().drain(batch);

    ASSERT_NE(marker, Collector::K_NO_THREAD);
    std::size_t marks = 0U;
    for (const ThreadDrain& thread : batch.threads)
    {
        if (thread.thread_index == marker)
            marks += thread.events.size();
    }
    EXPECT_EQ(marks, 1U);
}

TEST_F(Small_PeriodTimeline, ResetStartsPeriodZeroAgain)
{
    PeriodTimeline timeline(collector, "reset.step");
    timeline.mark();
    timeline.mark();
    timeline.reset();

    EXPECT_EQ(timeline.current(), PeriodTimeline::K_NO_PERIOD);
    EXPECT_EQ(timeline.mark(), 0U);
}
