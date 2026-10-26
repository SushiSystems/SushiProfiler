/**************************************************************************/
/* test_steady_state.cpp                                                  */
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
 * @file test_steady_state.cpp
 * @brief Tests that MSER-5 cuts a warm-up and leaves a steady series whole.
 * @author Mustafa Garip
 * @see tests/record/statistics/README.md
 */

#include <SushiProfiler/statistics/steady_state.hpp>

#include <gtest/gtest.h>

using SushiProfiler::mser5;

TEST(Small_SteadyState, CutsTheWarmUpAtTheEarliestBestBatch)
{
    std::vector<double> series(10U, 100.0);
    series.resize(50U, 10.0);

    EXPECT_EQ(mser5(series), 10U);
}

TEST(Small_SteadyState, LeavesASteadySeriesWhole)
{
    EXPECT_EQ(mser5(std::vector<double>(40U, 7.0)), 0U);
}

TEST(Small_SteadyState, NeedsTwoBatches)
{
    EXPECT_EQ(mser5(std::vector<double>(9U, 1.0)), 0U);
    EXPECT_EQ(mser5({}), 0U);
}

TEST(Small_SteadyState, IgnoresATrailingPartialBatch)
{
    std::vector<double> series(5U, 100.0);
    series.resize(23U, 10.0);
    series.push_back(1000.0);

    EXPECT_EQ(mser5(series), 5U);
}
