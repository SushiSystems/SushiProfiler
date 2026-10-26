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
 * @file recording_fixture.hpp
 * @brief Declares the fixture that opens the collector for a test and reads back its events.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#pragma once

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
} // namespace SushiProfilerTests
