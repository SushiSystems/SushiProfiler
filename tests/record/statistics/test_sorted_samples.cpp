/**************************************************************************/
/* test_sorted_samples.cpp                                                */
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
 * @file test_sorted_samples.cpp
 * @brief Tests order statistics and type-7 percentiles on fixed samples.
 * @author Mustafa Garip
 * @see tests/record/statistics/README.md
 */

#include <SushiProfiler/statistics/sorted_samples.hpp>

#include <gtest/gtest.h>

using SushiProfiler::SortedSamples;

TEST(Small_SortedSamples, SortsAndAnswersTheEnds)
{
    const SortedSamples samples({5.0, 1.0, 4.0, 2.0, 3.0});

    EXPECT_EQ(samples.size(), 5U);
    EXPECT_FALSE(samples.empty());
    EXPECT_EQ(samples.minimum(), 1.0);
    EXPECT_EQ(samples.maximum(), 5.0);
    EXPECT_EQ(samples.values(), (std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}));
}

TEST(Small_SortedSamples, TheMedianOfAnEvenCountIsTheMidpoint)
{
    EXPECT_EQ(SortedSamples({5.0, 1.0, 4.0, 2.0, 3.0}).median(), 3.0);
    EXPECT_EQ(SortedSamples({4.0, 1.0, 3.0, 2.0}).median(), 2.5);
}

TEST(Small_SortedSamples, PercentilesInterpolateByTheType7Rule)
{
    const SortedSamples samples({5.0, 1.0, 4.0, 2.0, 3.0});

    EXPECT_EQ(samples.percentile(0.0), 1.0);
    EXPECT_EQ(samples.percentile(25.0), 2.0);
    EXPECT_NEAR(samples.percentile(90.0), 4.6, 1e-12);
    EXPECT_EQ(samples.percentile(100.0), 5.0);
}

TEST(Small_SortedSamples, OneSampleIsEveryPercentile)
{
    const SortedSamples samples({7.0});

    EXPECT_EQ(samples.percentile(0.0), 7.0);
    EXPECT_EQ(samples.median(), 7.0);
    EXPECT_EQ(samples.percentile(99.0), 7.0);
}
