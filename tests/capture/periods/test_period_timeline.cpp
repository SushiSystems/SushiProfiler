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
 * @file test_period_timeline.cpp
 * @brief Tests that marks number periods from 0, carry their index and budget, and tags attach.
 * @author Mustafa Garip
 * @see tests/capture/periods/README.md
 */

#include <SushiProfiler/periods/period_timeline.hpp>

#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/events/trace_event.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::DrainBatch;
using SushiProfiler::EventKind;
using SushiProfiler::PeriodTimeline;
using SushiProfiler::ThreadDrain;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief Opens the collector before each test, and closes and empties it after.
    class Small_PeriodTimeline : public ::testing::Test
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

            /// @brief Returns what the calling thread wrote since the last drain.
            static std::vector<TraceEvent> drain_mine()
            {
                DrainBatch batch;
                Collector::instance().drain(batch);
                for (ThreadDrain& thread : batch.threads)
                {
                    if (thread.thread_index == Collector::thread_index())
                        return std::move(thread.events);
                }
                return {};
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

TEST_F(Small_PeriodTimeline, MarksCountFromZero)
{
    PeriodTimeline timeline("train.step");

    EXPECT_EQ(timeline.current(), PeriodTimeline::K_NO_PERIOD);
    EXPECT_EQ(timeline.mark(), 0U);
    EXPECT_EQ(timeline.mark(), 1U);
    EXPECT_EQ(timeline.current(), 1U);
    EXPECT_EQ(timeline.name(), "train.step");
    EXPECT_EQ(Collector::instance().names().text(timeline.id()), "train.step");
}

TEST_F(Small_PeriodTimeline, AMarkCarriesItsIndexAndTimeline)
{
    PeriodTimeline timeline("engine.frame");
    timeline.mark();
    timeline.mark();
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(SushiProfiler::kind_of(events[1]), EventKind::PeriodMark);
    EXPECT_EQ(events[1].end, 1U);
    EXPECT_EQ(events[1].name, timeline.id());
}

TEST_F(Small_PeriodTimeline, AMarkCarriesItsBudget)
{
    PeriodTimeline timeline("audio.block", 2'666'667U);
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
    PeriodTimeline timeline("bench.iteration");
    Collector::instance().close();
    timeline.mark();
    timeline.mark();
    Collector::instance().close();
    DrainBatch discarded;
    Collector::instance().drain(discarded);
    ASSERT_TRUE(Collector::instance().open(CollectorSettings{}));

    EXPECT_EQ(timeline.mark(), 2U);
    const std::vector<TraceEvent> events = drain_mine();
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].end, 2U);
}

TEST_F(Small_PeriodTimeline, ATagCarriesKeyAndValue)
{
    PeriodTimeline timeline("train.step");
    timeline.mark();
    timeline.tag("variant", "fused");
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(SushiProfiler::kind_of(events[1]), EventKind::PeriodTag);
    EXPECT_EQ(events[1].name, timeline.id());
    const auto key = static_cast<SushiProfiler::NameID>(events[1].end >> 32U);
    const auto value = static_cast<SushiProfiler::NameID>(events[1].end & 0xFFFFFFFFU);
    EXPECT_EQ(Collector::instance().names().text(key), "variant");
    EXPECT_EQ(Collector::instance().names().text(value), "fused");
}

TEST_F(Small_PeriodTimeline, TwoTimelinesKeepTheirOwnIndex)
{
    PeriodTimeline steps("train.step");
    PeriodTimeline batches("data.batch");
    steps.mark();
    batches.mark();
    batches.mark();

    EXPECT_EQ(steps.current(), 0U);
    EXPECT_EQ(batches.current(), 1U);
}
