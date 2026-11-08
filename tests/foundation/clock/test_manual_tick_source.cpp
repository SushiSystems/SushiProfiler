/**************************************************************************/
/* test_manual_tick_source.cpp                                            */
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
 * @file test_manual_tick_source.cpp
 * @brief Tests that the in-memory tick source moves only by what advance() adds.
 * @author Mustafa Garip
 * @see tests/foundation/clock/README.md
 */

#include <SushiProfiler/clock/manual_tick_source.hpp>

#include <SushiProfiler/clock/calibration.hpp>

#include <gtest/gtest.h>

using SushiProfiler::CalibrationPoint;
using SushiProfiler::ManualTickSource;

TEST(Small_ManualTickSource, AdvanceMovesTheStampByTheElapsedNanoseconds)
{
    ManualTickSource clock(100U);

    clock.advance(25U);

    EXPECT_EQ(clock.now(), 125U);
}

TEST(Small_ManualTickSource, ASamplePairsTheStampWithItself)
{
    ManualTickSource clock(40U);
    clock.advance(2U);

    const CalibrationPoint point = clock.sample();

    EXPECT_EQ(point.ticks, 42U);
    EXPECT_EQ(point.steady_ns, 42U);
}
