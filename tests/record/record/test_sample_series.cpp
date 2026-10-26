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
