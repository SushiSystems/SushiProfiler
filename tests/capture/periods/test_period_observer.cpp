/**************************************************************************/
/* test_period_observer.cpp                                               */
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
 * @file test_period_observer.cpp
 * @brief Tests that an observer hears each mark before and after its write, and steers the gate.
 * @author Mustafa Garip
 * @see tests/capture/periods/README.md
 */

#include <SushiProfiler/periods/period_observer.hpp>

#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/periods/period_timeline.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::DrainBatch;
using SushiProfiler::PeriodObserver;
using SushiProfiler::PeriodTimeline;
using SushiProfiler::ThreadDrain;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief Opens the collector before each test, and closes and empties it after.
    class Small_PeriodObserver : public ::testing::Test
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

            /// @brief Returns the indices of the marks this thread wrote since the last drain.
            static std::vector<std::uint64_t> drained_marks()
            {
                DrainBatch batch;
                Collector::instance().drain(batch);
                std::vector<std::uint64_t> marks;
                for (const ThreadDrain& thread : batch.threads)
                {
                    if (thread.thread_index != Collector::thread_index())
                        continue;
                    for (const TraceEvent& event : thread.events)
                        marks.push_back(event.end);
                }
                return marks;
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

    /// @brief Records each call, and opens or closes the gate where it is told to.
    class ScriptedObserver final : public PeriodObserver
    {
        public:
            void before_mark(const PeriodTimeline&, std::uint64_t index) noexcept override
            {
                calls.push_back("before " + std::to_string(index));
                if (index == resume_at)
                    Collector::instance().resume();
            }

            void after_mark(const PeriodTimeline&, std::uint64_t index) noexcept override
            {
                calls.push_back("after " + std::to_string(index));
                if (index == pause_at)
                    Collector::instance().pause();
            }

            /// @brief The calls heard, in order.
            std::vector<std::string> calls;

            /// @brief The index whose mark resumes the gate before it is written.
            std::uint64_t resume_at = ~std::uint64_t{0U};

            /// @brief The index whose mark pauses the gate after it is written.
            std::uint64_t pause_at = ~std::uint64_t{0U};
    };
} // namespace

TEST_F(Small_PeriodObserver, HearsEachMarkBeforeAndAfter)
{
    PeriodTimeline timeline("observed");
    ScriptedObserver observer;
    timeline.observe(&observer);
    timeline.mark();
    timeline.mark();
    timeline.observe(nullptr);
    timeline.mark();

    const std::vector<std::string> expected = {"before 0", "after 0", "before 1", "after 1"};
    EXPECT_EQ(observer.calls, expected);
}

TEST_F(Small_PeriodObserver, AMarkThatResumesTheGateIsWritten)
{
    PeriodTimeline timeline("window");
    ScriptedObserver observer;
    observer.resume_at = 1U;
    timeline.observe(&observer);
    Collector::instance().pause();
    timeline.mark();
    timeline.mark();
    timeline.observe(nullptr);

    EXPECT_EQ(drained_marks(), std::vector<std::uint64_t>{1U});
}

TEST_F(Small_PeriodObserver, AMarkThatPausesTheGateIsStillWritten)
{
    PeriodTimeline timeline("window");
    ScriptedObserver observer;
    observer.pause_at = 1U;
    timeline.observe(&observer);
    timeline.mark();
    timeline.mark();
    timeline.mark();
    timeline.observe(nullptr);

    EXPECT_EQ(drained_marks(), (std::vector<std::uint64_t>{0U, 1U}));
}
