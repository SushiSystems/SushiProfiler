/**************************************************************************/
/* recording_fixture.hpp                                                  */
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
 * @file recording_fixture.hpp
 * @brief Declares the fixtures that open a collector for a test and read back its events.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#pragma once

#include <SushiProfiler/clock/manual_tick_source.hpp>
#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/events/trace_event.hpp>

#include <gtest/gtest.h>

#include <vector>

namespace SushiProfilerTests
{
    /// @brief Opens the collector before each test, and closes and empties it after.
    class RecordingFixture : public ::testing::Test
    {
        protected:
            void SetUp() override
            {
                close_and_empty();
                ASSERT_TRUE(SushiProfiler::Collector::instance().open(
                    SushiProfiler::CollectorSettings{}));
            }

            void TearDown() override
            {
                close_and_empty();
            }

            /// @brief Returns what the calling thread wrote since the last drain.
            static std::vector<SushiProfiler::TraceEvent> drain_mine()
            {
                SushiProfiler::DrainBatch batch;
                SushiProfiler::Collector::instance().drain(batch);
                for (SushiProfiler::ThreadDrain& thread : batch.threads)
                {
                    if (thread.thread_index == SushiProfiler::Collector::thread_index())
                        return std::move(thread.events);
                }
                return {};
            }

        private:
            /// @brief Closes the collector and discards what its rings hold.
            static void close_and_empty()
            {
                SushiProfiler::Collector::instance().close();
                SushiProfiler::DrainBatch discarded;
                SushiProfiler::Collector::instance().drain(discarded);
            }
    };

    /// @brief Gives each test its own open collector on a manual clock that starts at 100 ns.
    class OwnedRecordingFixture : public ::testing::Test
    {
        protected:
            void SetUp() override
            {
                ASSERT_TRUE(collector.open(SushiProfiler::CollectorSettings{}));
            }

            /// @brief Returns what the calling thread wrote since the last drain.
            std::vector<SushiProfiler::TraceEvent> drain_mine()
            {
                SushiProfiler::DrainBatch batch;
                collector.drain(batch);
                for (SushiProfiler::ThreadDrain& thread : batch.threads)
                {
                    if (thread.thread_index == collector.index_of_thread())
                        return std::move(thread.events);
                }
                return {};
            }

            SushiProfiler::ManualTickSource clock{100U};
            SushiProfiler::Collector collector{clock};
    };
} // namespace SushiProfilerTests
