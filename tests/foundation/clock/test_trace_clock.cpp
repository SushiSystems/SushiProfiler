/**************************************************************************/
/* test_trace_clock.cpp                                                   */
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
 * @file test_trace_clock.cpp
 * @brief Tests the trace clock's steady reading and its calibration samples.
 * @author Mustafa Garip
 * @see tests/foundation/clock/README.md
 */

#include <SushiProfiler/clock/trace_clock.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using SushiProfiler::CalibrationPoint;
using SushiProfiler::TraceClock;

namespace
{
    /// @brief Reads @p clock repeatedly and expects no reading to be smaller than the one before.
    void expect_readings_never_decrease(std::uint64_t (*clock)())
    {
        std::uint64_t previous = clock();
        for (int read = 0; read < 1000; ++read)
        {
            const std::uint64_t now = clock();
            ASSERT_GE(now, previous);
            previous = now;
        }
    }

    /// @brief Skips a test on a processor whose counter ticks are not invariant.
    class Medium_TraceClockCounter : public ::testing::Test
    {
        protected:
            void SetUp() override
            {
                if (!TraceClock::ticks_are_invariant())
                    GTEST_SKIP() << "this processor has no invariant TSC";
            }
    };
} // namespace

TEST(Medium_TraceClock, SteadyTimeNeverGoesBack)
{
    expect_readings_never_decrease(&TraceClock::steady_now_ns);
}

TEST_F(Medium_TraceClockCounter, TicksNeverGoBackWhenInvariant)
{
    expect_readings_never_decrease(&TraceClock::now);
}

TEST(Medium_TraceClock, SampleBracketsTheSteadyReading)
{
    const std::uint64_t before = TraceClock::steady_now_ns();
    const CalibrationPoint point = TraceClock::sample();
    const std::uint64_t after = TraceClock::steady_now_ns();

    EXPECT_GE(point.steady_ns, before);
    EXPECT_LE(point.steady_ns, after);
}
