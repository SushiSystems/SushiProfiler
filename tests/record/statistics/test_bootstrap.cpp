/**************************************************************************/
/* test_bootstrap.cpp                                                     */
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
 * @file test_bootstrap.cpp
 * @brief Tests that the bootstrap interval repeats and matches an independent computation.
 * @author Mustafa Garip
 * @see tests/record/statistics/README.md
 */

#include <SushiProfiler/statistics/bootstrap.hpp>

#include <gtest/gtest.h>

using SushiProfiler::bootstrap_median_interval;
using SushiProfiler::BootstrapSettings;
using SushiProfiler::Interval;

namespace
{
    /// @brief Ten step times with one outlier.
    const std::vector<double> K_STEPS = {10.0, 12.0, 11.0, 13.0, 50.0,
                                         12.0, 11.0, 14.0, 12.0, 13.0};
} // namespace

TEST(Small_Bootstrap, TheSameSeedGivesTheSameInterval)
{
    const Interval first = bootstrap_median_interval(K_STEPS);
    const Interval second = bootstrap_median_interval(K_STEPS);

    EXPECT_EQ(first.low, second.low);
    EXPECT_EQ(first.high, second.high);
}

TEST(Small_Bootstrap, TheDefaultSeedGivesThePinnedInterval)
{
    // The pinned ends come from a Python reimplementation of SplitMix64 and the index mapping.
    const Interval interval = bootstrap_median_interval(K_STEPS);

    EXPECT_EQ(interval.low, 11.0);
    EXPECT_EQ(interval.high, 13.5);
}

TEST(Small_Bootstrap, TheIntervalHoldsTheMedianAndIgnoresTheOutlier)
{
    BootstrapSettings settings;
    settings.seed = 42U;
    const Interval interval = bootstrap_median_interval(K_STEPS, settings);

    EXPECT_LE(interval.low, 12.0);
    EXPECT_GE(interval.high, 12.0);
    EXPECT_LT(interval.high, 50.0);
}

TEST(Small_Bootstrap, OneSampleGivesAPointInterval)
{
    const Interval interval = bootstrap_median_interval({4.0});

    EXPECT_EQ(interval.low, 4.0);
    EXPECT_EQ(interval.high, 4.0);
}
