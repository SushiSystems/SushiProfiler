/**************************************************************************/
/* test_dispersion.cpp                                                    */
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
 * @file test_dispersion.cpp
 * @brief Tests the mean, the coefficient of variation and the jitter on fixed samples.
 * @author Mustafa Garip
 * @see tests/record/statistics/README.md
 */

#include <SushiProfiler/statistics/dispersion.hpp>

#include <gtest/gtest.h>

#include <cmath>

using SushiProfiler::coefficient_of_variation;
using SushiProfiler::jitter;
using SushiProfiler::mean;

TEST(Small_Dispersion, TheMeanOfNothingIsZero)
{
    EXPECT_EQ(mean({}), 0.0);
    EXPECT_EQ(mean({1.0, 2.0, 6.0}), 3.0);
}

TEST(Small_Dispersion, TheCoefficientOfVariationUsesTheSampleDeviation)
{
    const double expected = std::sqrt(32.0 / 7.0) / 5.0;

    EXPECT_NEAR(coefficient_of_variation({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}), expected,
                1e-15);
}

TEST(Small_Dispersion, TheCoefficientOfVariationIsZeroWithoutASpread)
{
    EXPECT_EQ(coefficient_of_variation({3.0}), 0.0);
    EXPECT_EQ(coefficient_of_variation({-1.0, 1.0}), 0.0);
    EXPECT_EQ(coefficient_of_variation({4.0, 4.0, 4.0}), 0.0);
}

TEST(Small_Dispersion, JitterIsTheMedianSuccessiveStep)
{
    EXPECT_EQ(jitter({10.0, 12.0, 11.0, 15.0, 15.0}), 1.5);
    EXPECT_EQ(jitter({1.0, 2.0, 3.0, 4.0, 5.0}), 1.0);
    EXPECT_EQ(jitter({8.0}), 0.0);
}
