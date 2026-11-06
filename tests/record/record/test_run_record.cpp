/**************************************************************************/
/* test_run_record.cpp                                                    */
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
 * @file test_run_record.cpp
 * @brief Tests the run record's lookups and its equality.
 * @author Mustafa Garip
 * @see tests/record/record/README.md
 */

#include <SushiProfiler/record/run_record.hpp>

#include <gtest/gtest.h>

using SushiProfiler::RunRecord;
using SushiProfiler::SiteRecord;

namespace
{
    /// @brief Returns a record with two names and one site.
    RunRecord small_record()
    {
        RunRecord record;
        record.names = {"", "train.step", "conv"};
        record.sites.resize(2U);
        record.sites[1].name = "forward";
        return record;
    }
} // namespace

TEST(Small_RunRecord, NamesAndSitesAreLookedUpById)
{
    const RunRecord record = small_record();

    EXPECT_EQ(record.name_of(1U), "train.step");
    EXPECT_EQ(record.name_of(9U), "");
    ASSERT_NE(record.site(1U), nullptr);
    EXPECT_EQ(record.site(1U)->name, "forward");
    EXPECT_EQ(record.site(0U), nullptr);
    EXPECT_EQ(record.site(2U), nullptr);
}

TEST(Small_RunRecord, FindNameReturnsTheIdOrZero)
{
    const RunRecord record = small_record();

    EXPECT_EQ(record.find_name("conv"), 2U);
    EXPECT_EQ(record.find_name("missing"), 0U);
    EXPECT_EQ(record.find_name(""), 0U);
}

TEST(Small_RunRecord, EqualityReachesEveryTable)
{
    const RunRecord first = small_record();
    RunRecord second = first;

    EXPECT_EQ(first, second);
    second.sites[1].group = "Host";
    EXPECT_FALSE(first == second);
    second = first;
    second.drops.open_at_stop = 1U;
    EXPECT_FALSE(first == second);
    second = first;
    second.clock.source = "tsc";
    EXPECT_FALSE(first == second);
}
