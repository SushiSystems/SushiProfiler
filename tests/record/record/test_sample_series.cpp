/**************************************************************************/
/* test_sample_series.cpp                                                 */
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
 * @file test_sample_series.cpp
 * @brief Tests that sample series compare every column.
 * @author Mustafa Garip
 * @see tests/record/record/README.md
 */

#include <SushiProfiler/record/sample_series.hpp>

#include <gtest/gtest.h>

using SushiProfiler::FlowPoint;
using SushiProfiler::SampleSeries;

TEST(Small_SampleSeries, EqualityComparesEveryColumn)
{
    SampleSeries first;
    first.site = 3U;
    first.time_ns = {1U, 2U};
    first.items = {7U, 7U};
    first.points = {FlowPoint::Begin, FlowPoint::End};
    SampleSeries second = first;

    EXPECT_EQ(first, second);
    second.points[1] = FlowPoint::Step;
    EXPECT_FALSE(first == second);
}
