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
 * @file test_period_retention.cpp
 * @brief Tests that the period list is trimmed in batches while running and exactly at the end.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/period_retention.hpp>

#include <gtest/gtest.h>

#include <cstdint>
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

    /// @brief Returns the index of each of @p periods, in order.
    std::vector<std::uint64_t> indexes_of(const std::vector<PeriodRecord>& periods)
    {
        std::vector<std::uint64_t> indexes;
        for (const PeriodRecord& period : periods)
            indexes.push_back(period.index);
        return indexes;
    }

    /// @brief A list of @p total periods, the count to keep and the indexes that must remain.
    struct FinalTrimRow
    {
        std::uint64_t total;
        std::uint32_t keep;
        std::vector<std::uint64_t> remaining;
    };
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

class Small_PeriodRetentionFinalTrim : public ::testing::TestWithParam<FinalTrimRow>
{
};

TEST_P(Small_PeriodRetentionFinalTrim, FinalTrimKeepsTheNewestNInOrder)
{
    std::vector<PeriodRecord> periods = numbered(GetParam().total);

    trim_periods(periods, GetParam().keep, true);

    EXPECT_EQ(indexes_of(periods), GetParam().remaining);
}

INSTANTIATE_TEST_SUITE_P(
    Small_Lists, Small_PeriodRetentionFinalTrim,
    ::testing::Values(FinalTrimRow{5U, 2U, {3U, 4U}}, FinalTrimRow{10U, 4U, {6U, 7U, 8U, 9U}}));

TEST(Small_PeriodRetention, AShorterListThanNIsLeftAlone)
{
    std::vector<PeriodRecord> periods = numbered(2U);

    trim_periods(periods, 10U, true);

    EXPECT_EQ(periods.size(), 2U);
}
