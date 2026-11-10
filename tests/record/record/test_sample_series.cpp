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

namespace
{
    /// @brief Returns a series whose every column differs from the default.
    SampleSeries full_series()
    {
        SampleSeries series;
        series.site = 3U;
        series.thread = 1U;
        series.time_ns = {1U, 2U};
        series.values = {0.5, 1.5};
        series.items = {7U, 7U};
        series.points = {FlowPoint::Begin, FlowPoint::End};
        return series;
    }

    /// @brief A column of a series and a change to it alone.
    struct ColumnChange
    {
        const char* column;
        void (*change)(SampleSeries&);
    };
} // namespace

TEST(Small_SampleSeries, ACopyIsEqual)
{
    const SampleSeries first = full_series();
    const SampleSeries second = first;

    EXPECT_EQ(first, second);
}

class Small_SampleSeriesColumn : public ::testing::TestWithParam<ColumnChange>
{
};

TEST_P(Small_SampleSeriesColumn, EqualityReachesTheColumn)
{
    const SampleSeries first = full_series();
    SampleSeries second = first;
    GetParam().change(second);

    EXPECT_FALSE(first == second) << GetParam().column;
}

INSTANTIATE_TEST_SUITE_P(
    Small_Columns, Small_SampleSeriesColumn,
    ::testing::Values(
        ColumnChange{"site", [](SampleSeries& series) { series.site = 9U; }},
        ColumnChange{"thread", [](SampleSeries& series) { series.thread = 9U; }},
        ColumnChange{"time_ns", [](SampleSeries& series) { series.time_ns[1] = 9U; }},
        ColumnChange{"values", [](SampleSeries& series) { series.values[1] = 9.0; }},
        ColumnChange{"items", [](SampleSeries& series) { series.items[1] = 9U; }},
        ColumnChange{"points",
                     [](SampleSeries& series) { series.points[1] = FlowPoint::Step; }}));
