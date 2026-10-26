/**************************************************************************/
/* test_check.cpp                                                         */
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
 * @file test_check.cpp
 * @brief Tests reading checks from JSON and each refusal with its path.
 * @author Mustafa Garip
 * @see tests/record/checks/README.md
 */

#include <SushiProfiler/checks/check.hpp>

#include <gtest/gtest.h>

using SushiProfiler::ChecksReadResult;
using SushiProfiler::read_checks;
using SushiProfiler::Statistic;

namespace
{
    /// @brief Returns the refusal of @p text.
    std::string refusal(const char* text)
    {
        const ChecksReadResult result = read_checks(text);
        EXPECT_FALSE(result.ok());
        EXPECT_TRUE(result.checks.empty());
        return result.error;
    }
} // namespace

TEST(Small_Check, ReadsTimeAndPlainChecks)
{
    const ChecksReadResult result = read_checks(R"([
        {"subject": "timeline:dsp.block", "statistic": "p99", "at_most_ns": 2670000},
        {"subject": "counter:bytes", "statistic": "max", "at_least": 1.5, "at_most": 9}
    ])");

    ASSERT_TRUE(result.ok()) << result.error;
    ASSERT_EQ(result.checks.size(), 2U);
    EXPECT_EQ(result.checks[0].subject, "timeline:dsp.block");
    EXPECT_EQ(result.checks[0].statistic, Statistic::P99);
    EXPECT_EQ(result.checks[0].at_most, 2670000.0);
    EXPECT_FALSE(result.checks[0].at_least.has_value());
    EXPECT_EQ(result.checks[1].statistic, Statistic::Max);
    EXPECT_EQ(result.checks[1].at_least, 1.5);
    EXPECT_EQ(result.checks[1].at_most, 9.0);
}

TEST(Small_Check, NamesEveryStatistic)
{
    EXPECT_EQ(SushiProfiler::statistic_word(Statistic::Median), "median");
    EXPECT_EQ(SushiProfiler::statistic_word(Statistic::P95), "p95");
    EXPECT_EQ(SushiProfiler::statistic_word(Statistic::Min), "min");
}

TEST(Small_Check, RefusesATimeBoundOnAPlainSubjectAndTheReverse)
{
    EXPECT_EQ(refusal(R"([{"subject": "zone:conv", "statistic": "median", "at_most": 5}])"),
              "checks[0]: unknown key \"at_most\" for this subject; its bounds are at_most_ns and "
              "at_least_ns");
    EXPECT_EQ(refusal(R"([{"subject": "value:loss", "statistic": "median", "at_most_ns": 5}])"),
              "checks[0]: unknown key \"at_most_ns\" for this subject; its bounds are at_most and "
              "at_least");
}

TEST(Small_Check, RefusesWhatTheFormatDoesNotHold)
{
    EXPECT_EQ(refusal(R"({"subject": "timeline:a"})"), "checks: an array of checks was expected");
    EXPECT_EQ(refusal(R"([{"subject": "frame:a", "statistic": "p99", "at_most_ns": 1}])"),
              "checks[0].subject: \"frame:a\" does not start with timeline:, group:, zone:, "
              "span:, flow:, counter: or value:");
    EXPECT_EQ(refusal(R"([{"subject": "zone:a", "statistic": "p42", "at_most_ns": 1}])"),
              "checks[0].statistic: not one of median, p50, p90, p95, p99, min, max");
    EXPECT_EQ(refusal(R"([{"subject": "zone:a", "at_most_ns": 1}])"),
              "checks[0]: missing key \"statistic\"");
    EXPECT_EQ(refusal(R"([{"subject": "zone:a", "statistic": "max"}])"),
              "checks[0]: a check needs at_most_ns or at_least_ns");
    EXPECT_EQ(refusal(R"([{"subject": "zone:a", "statistic": "max", "at_most_ns": "1"}])"),
              "checks[0].at_most_ns: a number was expected");
    EXPECT_EQ(refusal("[1"), "line 1, column 3: ',' or ']' was expected");
}
