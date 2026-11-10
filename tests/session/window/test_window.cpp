/**************************************************************************/
/* test_window.cpp                                                        */
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
 * @file test_window.cpp
 * @brief Tests the window's phases and that it writes the marks of recorded periods and one more.
 * @author Mustafa Garip
 * @see tests/session/window/README.md
 */

#include <SushiProfiler/window/window.hpp>

#include <gtest/gtest.h>

#include <vector>

#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/periods/period_timeline.hpp>

using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::DrainBatch;
using SushiProfiler::PeriodTimeline;
using SushiProfiler::ThreadDrain;
using SushiProfiler::TraceEvent;
using SushiProfiler::Window;
using SushiProfiler::WindowPhase;
using SushiProfiler::WindowSchedule;

namespace
{
    /// @brief Returns a schedule of one skipped, one waiting, one warm-up and two active periods.
    WindowSchedule short_schedule(std::uint32_t repeat)
    {
        WindowSchedule schedule;
        schedule.skip = 1U;
        schedule.wait = 1U;
        schedule.warmup = 1U;
        schedule.active = 2U;
        schedule.repeat = repeat;
        return schedule;
    }

    /// @brief Opens the collector before each test, and closes and empties it after.
    class Medium_Window : public ::testing::Test
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

            /// @brief Returns the period indices this thread's marks carried since the last drain.
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
} // namespace

namespace
{
    /// @brief A period index and the phase the window places it in.
    struct PhaseRow
    {
        std::uint64_t index;
        WindowPhase phase;
    };
} // namespace

class Small_WindowCycle : public ::testing::TestWithParam<PhaseRow>
{
};

TEST_P(Small_WindowCycle, APeriodIsPlacedInItsPhaseOfTheCycle)
{
    const Window window(short_schedule(2U));

    EXPECT_EQ(window.phase_of(GetParam().index), GetParam().phase);
}

INSTANTIATE_TEST_SUITE_P(
    Small_Periods, Small_WindowCycle,
    ::testing::Values(PhaseRow{0U, WindowPhase::Skipped}, PhaseRow{1U, WindowPhase::Waiting},
                      PhaseRow{2U, WindowPhase::Warmup}, PhaseRow{3U, WindowPhase::Active},
                      PhaseRow{4U, WindowPhase::Active}, PhaseRow{5U, WindowPhase::Waiting},
                      PhaseRow{6U, WindowPhase::Warmup}, PhaseRow{7U, WindowPhase::Active},
                      PhaseRow{8U, WindowPhase::Active}, PhaseRow{9U, WindowPhase::Done},
                      PhaseRow{10U, WindowPhase::Done}));

TEST(Small_WindowPhases, ARecordedPeriodIsRecordedAndAWaitingOneIsNot)
{
    const Window window(short_schedule(2U));

    EXPECT_TRUE(window.records(2U));
    EXPECT_FALSE(window.records(5U));
}

TEST(Small_WindowPhases, RepeatZeroNeverEnds)
{
    const Window window(short_schedule(0U));

    EXPECT_EQ(window.phase_of(1U + 5U * 1000U + 3U), WindowPhase::Active);
}

TEST_F(Medium_Window, WritesTheRecordedMarksAndTheOneAfter)
{
    Window window(short_schedule(1U));
    PeriodTimeline timeline("windowed");
    timeline.observe(&window);
    Collector::instance().pause();
    for (int period = 0; period < 7; ++period)
        static_cast<void>(timeline.mark());
    timeline.observe(nullptr);

    EXPECT_EQ(drained_marks(), (std::vector<std::uint64_t>{2U, 3U, 4U, 5U}));
    EXPECT_FALSE(Collector::recording());
}

namespace
{
    /// @brief Returns a schedule that skips 1, waits 1, warms up 2 and records every period after.
    WindowSchedule active_all_after_a_lead_in()
    {
        WindowSchedule schedule;
        schedule.skip = 1U;
        schedule.wait = 1U;
        schedule.warmup = 2U;
        schedule.active_all = true;
        return schedule;
    }
} // namespace

class Small_WindowActiveAll : public ::testing::TestWithParam<PhaseRow>
{
};

TEST_P(Small_WindowActiveAll, SkipWaitAndWarmupApplyOnceThenEveryPeriodIsActive)
{
    const Window window(active_all_after_a_lead_in());

    EXPECT_EQ(window.phase_of(GetParam().index), GetParam().phase);
}

INSTANTIATE_TEST_SUITE_P(
    Small_Periods, Small_WindowActiveAll,
    ::testing::Values(PhaseRow{0U, WindowPhase::Skipped}, PhaseRow{1U, WindowPhase::Waiting},
                      PhaseRow{2U, WindowPhase::Warmup}, PhaseRow{3U, WindowPhase::Warmup},
                      PhaseRow{4U, WindowPhase::Active}, PhaseRow{5U, WindowPhase::Active},
                      PhaseRow{99'994U, WindowPhase::Active},
                      PhaseRow{10'000'000U, WindowPhase::Active},
                      PhaseRow{std::uint64_t{1} << 40U, WindowPhase::Active}));

TEST(Small_WindowPhases, ActiveAllRecordsFromTheEndOfTheLeadIn)
{
    const Window window(active_all_after_a_lead_in());

    EXPECT_TRUE(window.records(2U));
    EXPECT_FALSE(window.records(1U));
}

TEST(Small_WindowPhases, ActiveAllRecordsFromTheFirstPeriodWithNoWarmup)
{
    WindowSchedule schedule;
    schedule.warmup = 0U;
    schedule.active_all = true;
    const Window window(schedule);

    EXPECT_EQ(window.phase_of(0U), WindowPhase::Active);
    EXPECT_TRUE(window.records(0U));
}
