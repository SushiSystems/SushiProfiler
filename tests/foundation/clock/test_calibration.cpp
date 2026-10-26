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
 * @file test_calibration.cpp
 * @brief Tests the line that turns clock ticks into steady nanoseconds.
 * @author Mustafa Garip
 * @see tests/foundation/clock/README.md
 */

#include <SushiProfiler/clock/calibration.hpp>

#include <gtest/gtest.h>

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

TEST(Small_Calibration, MapsBothEndsExactly)
{
    const Calibration calibration = half_nanosecond_per_tick();

    EXPECT_EQ(calibration.to_nanoseconds(1000U), 5000U);
    EXPECT_EQ(calibration.to_nanoseconds(3000U), 6000U);
    EXPECT_EQ(calibration.to_nanoseconds(2000U), 5500U);
    EXPECT_DOUBLE_EQ(calibration.nanoseconds_per_tick(), 0.5);
}

TEST(Small_Calibration, ExtrapolatesBeforeTheBegin)
{
    EXPECT_EQ(half_nanosecond_per_tick().to_nanoseconds(500U), 4750U);
}

TEST(Small_Calibration, EqualTicksKeepOneNanosecondPerTick)
{
    const Calibration calibration(CalibrationPoint{100U, 900U}, CalibrationPoint{100U, 950U});

    EXPECT_DOUBLE_EQ(calibration.nanoseconds_per_tick(), 1.0);
    EXPECT_EQ(calibration.to_nanoseconds(110U), 910U);
}

TEST(Small_Calibration, DefaultIsTheIdentity)
{
    EXPECT_EQ(Calibration().to_nanoseconds(123456789U), 123456789U);
}
