/**************************************************************************/
/* test_verdict.cpp                                                       */
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
 * @file test_verdict.cpp
 * @brief Tests verdicts within, above and below their bounds, and on a missing subject.
 * @author Mustafa Garip
 * @see tests/record/checks/README.md
 */

#include <SushiProfiler/checks/verdict.hpp>

#include <gtest/gtest.h>

#include <cstddef>

#include "support/tolerance.hpp"

using SushiProfiler::Check;
using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::Statistic;
using SushiProfiler::Verdict;

namespace
{
    /**
     * @brief Rounded operations behind a type-7 percentile: the rank, its fraction and the
     * interpolation.
     */
    constexpr std::size_t K_PERCENTILE_OPERATIONS = 6U;

    /// @brief Returns a record whose timeline `step` has periods of 100, 120 and 110 ns.
    RunRecord step_record()
    {
        RunRecord record;
        record.names = {"", "step"};
        for (const std::uint64_t length : {100U, 120U, 110U})
        {
            PeriodRecord period;
            period.timeline = 1U;
            period.end_ns = length;
            record.periods.push_back(period);
        }
        return record;
    }

    /// @brief Returns step_record() with a 500 ns warm-up period before its periods.
    RunRecord step_record_with_warmup()
    {
        RunRecord record = step_record();
        PeriodRecord warmup;
        warmup.timeline = 1U;
        warmup.end_ns = 500U;
        warmup.warmup = true;
        record.periods.insert(record.periods.begin(), warmup);
        return record;
    }

    /// @brief Returns a check of @p statistic of timeline `step`.
    Check step_check(Statistic statistic, std::optional<double> at_most,
                     std::optional<double> at_least = std::nullopt)
    {
        Check check;
        check.subject = "timeline:step";
        check.statistic = statistic;
        check.at_most = at_most;
        check.at_least = at_least;
        return check;
    }
} // namespace

TEST(Small_Verdict, PassesWithinTheBounds)
{
    const std::vector<Verdict> verdicts =
        SushiProfiler::check(step_record(), {step_check(Statistic::Median, 110.0, 100.0)});

    ASSERT_EQ(verdicts.size(), 1U);
    EXPECT_TRUE(verdicts[0].passed);
    EXPECT_EQ(verdicts[0].value, 110.0);
    EXPECT_TRUE(verdicts[0].reason.empty());
    EXPECT_TRUE(SushiProfiler::all_passed(verdicts));
}

TEST(Small_Verdict, FailsAboveAtMostAndBelowAtLeast)
{
    const std::vector<Verdict> verdicts =
        SushiProfiler::check(step_record(), {step_check(Statistic::Max, 115.0),
                                             step_check(Statistic::Min, std::nullopt, 105.0)});

    ASSERT_EQ(verdicts.size(), 2U);
    EXPECT_FALSE(verdicts[0].passed);
    EXPECT_EQ(verdicts[0].reason, "max is 120 ns, above at most 115 ns");
    EXPECT_FALSE(verdicts[1].passed);
    EXPECT_EQ(verdicts[1].reason, "min is 100 ns, below at least 105 ns");
    EXPECT_FALSE(SushiProfiler::all_passed(verdicts));
}

TEST(Small_Verdict, PercentilesFollowTheType7Rule)
{
    const std::vector<Verdict> verdicts =
        SushiProfiler::check(step_record(), {step_check(Statistic::P90, 200.0)});

    EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_relative_error<double>(
        verdicts[0].value, 118.0, K_PERCENTILE_OPERATIONS)));
}

TEST(Small_Verdict, ACheckReadsTheActivePeriodsOnly)
{
    const std::vector<Verdict> verdicts =
        SushiProfiler::check(step_record_with_warmup(), {step_check(Statistic::Max, 125.0)});

    ASSERT_EQ(verdicts.size(), 1U);
    EXPECT_TRUE(verdicts[0].passed);
    EXPECT_EQ(verdicts[0].value, 120.0);
}

TEST(Small_Verdict, TheFiftiethPercentileGivesTheMedian)
{
    const std::vector<Verdict> verdicts = SushiProfiler::check(
        step_record(), {step_check(Statistic::Median, 200.0), step_check(Statistic::P50, 200.0)});

    ASSERT_EQ(verdicts.size(), 2U);
    EXPECT_EQ(verdicts[1].value, verdicts[0].value);
}

TEST(Small_Verdict, AMissingSubjectFails)
{
    Check check = step_check(Statistic::Median, 1.0);
    check.subject = "zone:missing";
    const std::vector<Verdict> verdicts = SushiProfiler::check(step_record(), {check});

    EXPECT_FALSE(verdicts[0].passed);
    EXPECT_EQ(verdicts[0].reason, "the record has no samples of zone:missing");
}
