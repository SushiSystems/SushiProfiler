/**************************************************************************/
/* test_calibration.cpp                                                   */
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
 * @file test_calibration.cpp
 * @brief Tests the line that turns clock ticks into steady nanoseconds.
 * @author Mustafa Garip
 * @see tests/foundation/clock/README.md
 */

#include <SushiProfiler/clock/calibration.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using SushiProfiler::Calibration;
using SushiProfiler::CalibrationPoint;

namespace
{
    /// @brief Two points two thousand ticks and one thousand nanoseconds apart.
    Calibration half_nanosecond_per_tick()
    {
        return Calibration(CalibrationPoint{1000U, 5000U}, CalibrationPoint{3000U, 6000U});
    }
} // namespace

TEST(Small_Calibration, MapsTheFirstPointToItsNanosecondsExactly)
{
    EXPECT_EQ(half_nanosecond_per_tick().to_nanoseconds(1000U), 5000U);
}

TEST(Small_Calibration, MapsTheSecondPointToItsNanosecondsExactly)
{
    EXPECT_EQ(half_nanosecond_per_tick().to_nanoseconds(3000U), 6000U);
}

TEST(Small_Calibration, MapsTicksBetweenThePointsAlongTheirLine)
{
    EXPECT_EQ(half_nanosecond_per_tick().to_nanoseconds(2000U), 5500U);
}

TEST(Small_Calibration, ReportsTheSlopeOfTheLine)
{
    EXPECT_DOUBLE_EQ(half_nanosecond_per_tick().nanoseconds_per_tick(), 0.5);
}

TEST(Small_Calibration, ExtrapolatesBeforeTheBegin)
{
    // The line is 0.5 ns per tick through (1000, 5000): 500 ticks give 5000 - 500 * 0.5 = 4750.
    EXPECT_EQ(half_nanosecond_per_tick().to_nanoseconds(500U), 4750U);
}

TEST(Small_Calibration, ExtrapolatesPastTheEndAlongTheSameLine)
{
    // The same line: 4000 ticks give 6000 + 1000 * 0.5 = 6500.
    EXPECT_EQ(half_nanosecond_per_tick().to_nanoseconds(4000U), 6500U);
}

TEST(Small_Calibration, EqualTicksKeepOneNanosecondPerTick)
{
    const Calibration calibration(CalibrationPoint{100U, 900U}, CalibrationPoint{100U, 950U});

    EXPECT_DOUBLE_EQ(calibration.nanoseconds_per_tick(), 1.0);
    EXPECT_EQ(calibration.to_nanoseconds(110U), 910U);
}

class Small_CalibrationIdentity : public ::testing::TestWithParam<std::uint64_t>
{
};

TEST_P(Small_CalibrationIdentity, ADefaultCalibrationMapsATickToTheSameNanoseconds)
{
    EXPECT_EQ(Calibration().to_nanoseconds(GetParam()), GetParam());
}

INSTANTIATE_TEST_SUITE_P(Small_Ticks, Small_CalibrationIdentity,
                         ::testing::Values(0U, 1U, 123456789U, 1'000'000'000'000U));
