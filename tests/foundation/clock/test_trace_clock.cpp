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

TEST(Small_TraceClock, SteadyTimeNeverGoesBack)
{
    std::uint64_t previous = TraceClock::steady_now_ns();
    for (int read = 0; read < 1000; ++read)
    {
        const std::uint64_t now = TraceClock::steady_now_ns();
        ASSERT_GE(now, previous);
        previous = now;
    }
}

TEST(Small_TraceClock, TicksNeverGoBackWhenInvariant)
{
    if (!TraceClock::ticks_are_invariant())
        GTEST_SKIP() << "this processor has no invariant TSC";
    std::uint64_t previous = TraceClock::now();
    for (int read = 0; read < 1000; ++read)
    {
        const std::uint64_t now = TraceClock::now();
        ASSERT_GE(now, previous);
        previous = now;
    }
}

TEST(Small_TraceClock, SampleBracketsTheSteadyReading)
{
    const std::uint64_t before = TraceClock::steady_now_ns();
    const CalibrationPoint point = TraceClock::sample();
    const std::uint64_t after = TraceClock::steady_now_ns();

    EXPECT_GE(point.steady_ns, before);
    EXPECT_LE(point.steady_ns, after);
}
