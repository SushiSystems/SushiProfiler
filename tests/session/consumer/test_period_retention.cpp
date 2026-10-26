/**************************************************************************/
/* test_period_retention.cpp                                              */
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
 * @file test_period_retention.cpp
 * @brief Tests that the period list is trimmed in batches while running and exactly at the end.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/period_retention.hpp>

#include <gtest/gtest.h>

#include <vector>

using SushiProfiler::PeriodRecord;
using SushiProfiler::trim_periods;

namespace
{
    /// @brief Returns @p count periods numbered from 0.
    std::vector<PeriodRecord> numbered(std::uint64_t count)
    {
        std::vector<PeriodRecord> periods(count);
        for (std::uint64_t index = 0U; index < count; ++index)
            periods[index].index = index;
        return periods;
    }
} // namespace

TEST(Small_PeriodRetention, AbsentKeepsEverything)
{
    std::vector<PeriodRecord> periods = numbered(50U);

    trim_periods(periods, std::nullopt, false);
    trim_periods(periods, std::nullopt, true);

    EXPECT_EQ(periods.size(), 50U);
}

TEST(Small_PeriodRetention, ZeroClearsAtOnce)
{
    std::vector<PeriodRecord> periods = numbered(3U);

    trim_periods(periods, 0U, false);

    EXPECT_TRUE(periods.empty());
}

TEST(Small_PeriodRetention, TrimsToNWhenTheListReachesTwoN)
{
    std::vector<PeriodRecord> periods = numbered(5U);
    trim_periods(periods, 3U, false);
    EXPECT_EQ(periods.size(), 5U);

    periods.push_back(PeriodRecord{});
    periods.back().index = 5U;
    trim_periods(periods, 3U, false);

    ASSERT_EQ(periods.size(), 3U);
    EXPECT_EQ(periods.front().index, 3U);
}

TEST(Small_PeriodRetention, FinalTrimLeavesExactlyN)
{
    std::vector<PeriodRecord> periods = numbered(5U);

    trim_periods(periods, 2U, true);

    ASSERT_EQ(periods.size(), 2U);
    EXPECT_EQ(periods[0].index, 3U);
    EXPECT_EQ(periods[1].index, 4U);
}

TEST(Small_PeriodRetention, KeepsTheNewestPeriodsInOrder)
{
    std::vector<PeriodRecord> periods = numbered(10U);

    trim_periods(periods, 4U, true);

    for (std::size_t slot = 0U; slot < periods.size(); ++slot)
        EXPECT_EQ(periods[slot].index, 6U + slot);
}

TEST(Small_PeriodRetention, AShorterListThanNIsLeftAlone)
{
    std::vector<PeriodRecord> periods = numbered(2U);

    trim_periods(periods, 10U, true);

    EXPECT_EQ(periods.size(), 2U);
}
