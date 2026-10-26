/**************************************************************************/
/* test_log2_histogram.cpp                                                */
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
 * @file test_log2_histogram.cpp
 * @brief Tests power-of-two binning, exact at each power, with empty bins kept.
 * @author Mustafa Garip
 * @see tests/record/statistics/README.md
 */

#include <SushiProfiler/statistics/log2_histogram.hpp>

#include <gtest/gtest.h>

using SushiProfiler::Log2Histogram;
using SushiProfiler::log2_histogram;

TEST(Small_Log2Histogram, BinsByPowerOfTwoAndKeepsEmptyBins)
{
    const Log2Histogram histogram =
        log2_histogram({0.0, 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 1023.0, 1024.0});

    EXPECT_EQ(histogram.nonpositive, 1U);
    EXPECT_EQ(histogram.first_exponent, -1);
    const std::vector<std::uint64_t> expected = {1U, 2U, 2U, 1U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 1U};
    EXPECT_EQ(histogram.counts, expected);
}

TEST(Small_Log2Histogram, NoPositiveSampleGivesNoBins)
{
    const Log2Histogram histogram = log2_histogram({0.0, -3.0});

    EXPECT_EQ(histogram.nonpositive, 2U);
    EXPECT_TRUE(histogram.counts.empty());
    EXPECT_EQ(histogram, (Log2Histogram{2U, 0, {}}));
}
