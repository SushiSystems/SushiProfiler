/**************************************************************************/
/* test_tick_source.cpp                                                   */
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
 * @file test_tick_source.cpp
 * @brief Tests that every TickSource implementation keeps the promises of the port.
 * @author Mustafa Garip
 * @see tests/foundation/clock/README.md
 */

#include <SushiProfiler/clock/tick_source.hpp>

#include <SushiProfiler/clock/calibration.hpp>
#include <SushiProfiler/clock/manual_tick_source.hpp>
#include <SushiProfiler/clock/trace_tick_source.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using SushiProfiler::CalibrationPoint;
using SushiProfiler::ManualTickSource;
using SushiProfiler::TickSource;
using SushiProfiler::TraceTickSource;

namespace
{
    /// @brief Expects a stamp read after another from @p clock not to be smaller.
    void expect_stamps_do_not_decrease(const TickSource& clock)
    {
        const std::uint64_t first = clock.now();
        const std::uint64_t second = clock.now();

        EXPECT_LE(first, second);
    }

    /// @brief Expects the stamp of a sample of @p clock to lie between stamps read around it.
    void expect_sample_between_stamps(const TickSource& clock)
    {
        const std::uint64_t before = clock.now();
        const CalibrationPoint point = clock.sample();
        const std::uint64_t after = clock.now();

        EXPECT_LE(before, point.ticks);
        EXPECT_LE(point.ticks, after);
    }
} // namespace

TEST(Small_ManualTickSourceContract, KeepsStampsFromDecreasing)
{
    const ManualTickSource clock(7U);

    expect_stamps_do_not_decrease(clock);
}

TEST(Small_ManualTickSourceContract, KeepsASampleBetweenTheStampsAroundIt)
{
    const ManualTickSource clock(7U);

    expect_sample_between_stamps(clock);
}

TEST(Medium_TraceTickSourceContract, KeepsStampsFromDecreasing)
{
    const TraceTickSource clock;

    expect_stamps_do_not_decrease(clock);
}

TEST(Medium_TraceTickSourceContract, KeepsASampleBetweenTheStampsAroundIt)
{
    const TraceTickSource clock;

    expect_sample_between_stamps(clock);
}
