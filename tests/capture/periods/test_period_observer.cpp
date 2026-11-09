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
 * @file test_period_observer.cpp
 * @brief Tests that an observer hears each mark before and after its write, and steers the gate.
 * @author Mustafa Garip
 * @see tests/capture/periods/README.md
 */

#include <SushiProfiler/periods/period_observer.hpp>

#include <SushiProfiler/clock/manual_tick_source.hpp>
#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/periods/period_timeline.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::DrainBatch;
using SushiProfiler::ManualTickSource;
using SushiProfiler::PeriodObserver;
using SushiProfiler::PeriodTimeline;
using SushiProfiler::ThreadDrain;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief The index no mark carries, so an observer left at it never steers the gate.
    constexpr std::uint64_t K_NEVER = ~std::uint64_t{0U};

    /// @brief Gives each test its own open collector on a manual clock that starts at 100 ns.
    class Small_PeriodObserver : public ::testing::Test
    {
        protected:
            void SetUp() override
            {
                ASSERT_TRUE(collector.open(CollectorSettings{}));
            }

            /// @brief Returns the indices of the marks this thread wrote since the last drain.
            std::vector<std::uint64_t> drained_marks()
            {
                DrainBatch batch;
                collector.drain(batch);
                std::vector<std::uint64_t> marks;
                for (const ThreadDrain& thread : batch.threads)
                {
                    if (thread.thread_index != collector.index_of_thread())
                        continue;
                    for (const TraceEvent& event : thread.events)
                        marks.push_back(event.end);
                }
                return marks;
            }

            ManualTickSource clock{100U};
            Collector collector{clock};
    };

    /// @brief Records the index of each call, and steers the gate of its collector where told to.
    class ScriptedObserver final : public PeriodObserver
    {
        public:
            /// @brief Builds an observer that steers @p collector.
            explicit ScriptedObserver(Collector& collector) : collector_(collector)
            {
            }

            void before_mark(const PeriodTimeline&, std::uint64_t index) noexcept override
            {
                before_indices.push_back(index);
                if (index == resume_at)
                    collector_.resume();
            }

            void after_mark(const PeriodTimeline&, std::uint64_t index) noexcept override
            {
                after_indices.push_back(index);
                if (index == pause_at)
                    collector_.pause();
            }

            /// @brief The indices heard before a write, in order.
            std::vector<std::uint64_t> before_indices;

            /// @brief The indices heard after a write, in order.
            std::vector<std::uint64_t> after_indices;

            /// @brief The index whose mark resumes the gate before it is written.
            std::uint64_t resume_at = K_NEVER;

            /// @brief The index whose mark pauses the gate after it is written.
            std::uint64_t pause_at = K_NEVER;

        private:
            /// @brief The collector whose gate the observer steers.
            Collector& collector_;
    };
} // namespace

TEST_F(Small_PeriodObserver, HearsEachMarkBeforeItIsWritten)
{
    PeriodTimeline timeline(collector, "observed");
    ScriptedObserver observer(collector);
    timeline.observe(&observer);

    timeline.mark();
    timeline.mark();

    EXPECT_EQ(observer.before_indices, (std::vector<std::uint64_t>{0U, 1U}));
}

TEST_F(Small_PeriodObserver, HearsEachMarkAfterItIsWritten)
{
    PeriodTimeline timeline(collector, "observed");
    ScriptedObserver observer(collector);
    timeline.observe(&observer);

    timeline.mark();
    timeline.mark();

    EXPECT_EQ(observer.after_indices, (std::vector<std::uint64_t>{0U, 1U}));
}

TEST_F(Small_PeriodObserver, HearsNoMarkAfterItIsRemoved)
{
    PeriodTimeline timeline(collector, "observed");
    ScriptedObserver observer(collector);
    timeline.observe(&observer);
    timeline.mark();

    timeline.observe(nullptr);
    timeline.mark();

    EXPECT_EQ(observer.before_indices, std::vector<std::uint64_t>{0U});
    EXPECT_EQ(observer.after_indices, std::vector<std::uint64_t>{0U});
}

TEST_F(Small_PeriodObserver, AMarkThatResumesTheGateIsWritten)
{
    PeriodTimeline timeline(collector, "window");
    ScriptedObserver observer(collector);
    observer.resume_at = 1U;
    timeline.observe(&observer);
    collector.pause();
    timeline.mark();
    timeline.mark();
    timeline.observe(nullptr);

    EXPECT_EQ(drained_marks(), std::vector<std::uint64_t>{1U});
}

TEST_F(Small_PeriodObserver, AMarkThatPausesTheGateIsStillWritten)
{
    PeriodTimeline timeline(collector, "window");
    ScriptedObserver observer(collector);
    observer.pause_at = 1U;
    timeline.observe(&observer);
    timeline.mark();
    timeline.mark();
    timeline.mark();
    timeline.observe(nullptr);

    EXPECT_EQ(drained_marks(), (std::vector<std::uint64_t>{0U, 1U}));
}
