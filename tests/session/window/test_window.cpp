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
    class Small_Window : public ::testing::Test
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

TEST(Small_WindowPhases, PlacesEachPeriodInItsCycle)
{
    const Window window(short_schedule(2U));
    const std::vector<WindowPhase> expected = {
        WindowPhase::Skipped, WindowPhase::Waiting, WindowPhase::Warmup, WindowPhase::Active,
        WindowPhase::Active,  WindowPhase::Waiting, WindowPhase::Warmup, WindowPhase::Active,
        WindowPhase::Active,  WindowPhase::Done,    WindowPhase::Done};

    for (std::uint64_t index = 0U; index < expected.size(); ++index)
        EXPECT_EQ(window.phase_of(index), expected[index]) << "period " << index;
    EXPECT_TRUE(window.records(2U));
    EXPECT_FALSE(window.records(5U));
}

TEST(Small_WindowPhases, RepeatZeroNeverEnds)
{
    const Window window(short_schedule(0U));

    EXPECT_EQ(window.phase_of(1U + 5U * 1000U + 3U), WindowPhase::Active);
}

TEST_F(Small_Window, WritesTheRecordedMarksAndTheOneAfter)
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

TEST(Small_WindowPhases, ActiveAllRecordsEveryPeriodAfterWarmup)
{
    WindowSchedule schedule;
    schedule.warmup = 3U;
    schedule.active_all = true;
    const Window window(schedule);

    for (std::uint64_t index = 3U; index < 10'000'000U; index += 99'991U)
        EXPECT_EQ(window.phase_of(index), WindowPhase::Active);
    EXPECT_EQ(window.phase_of(std::uint64_t{1} << 40U), WindowPhase::Active);
}

TEST(Small_WindowPhases, ActiveAllAppliesSkipWaitAndWarmupOnce)
{
    WindowSchedule schedule;
    schedule.skip = 1U;
    schedule.wait = 1U;
    schedule.warmup = 2U;
    schedule.active_all = true;
    const Window window(schedule);

    EXPECT_EQ(window.phase_of(0U), WindowPhase::Skipped);
    EXPECT_EQ(window.phase_of(1U), WindowPhase::Waiting);
    EXPECT_EQ(window.phase_of(2U), WindowPhase::Warmup);
    EXPECT_EQ(window.phase_of(3U), WindowPhase::Warmup);
    EXPECT_EQ(window.phase_of(4U), WindowPhase::Active);
    EXPECT_EQ(window.phase_of(5U), WindowPhase::Active);
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
