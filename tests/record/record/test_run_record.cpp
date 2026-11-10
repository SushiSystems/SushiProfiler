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
using SushiProfiler::ThreadRecord;

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

    /// @brief Returns a record whose every table and field differs from the default.
    RunRecord full_record()
    {
        RunRecord record = small_record();
        record.environment.push_back({"cpu", "x"});
        record.build.push_back({"type", "Release"});
        record.tags.push_back({"run", "a"});
        record.clock.source = "steady";
        record.clock.nanoseconds_per_tick = 2.0;
        record.clock.read_cost_ns = 3.0;
        record.tracks = {"", "queue 0"};
        record.threads.push_back(ThreadRecord{1U, "main", 2U, 3U, 4U});
        record.periods.resize(1U);
        record.drops = {1U, 2U, 3U};
        return record;
    }

    /// @brief A field of a record and a change to it alone.
    struct FieldChange
    {
        const char* field;
        void (*change)(RunRecord&);
    };
} // namespace

TEST(Small_RunRecord, ANameIsLookedUpById)
{
    const RunRecord record = small_record();

    EXPECT_EQ(record.name_of(1U), "train.step");
}

TEST(Small_RunRecord, AnUnknownNameIdGivesAnEmptyName)
{
    const RunRecord record = small_record();

    EXPECT_EQ(record.name_of(9U), "");
}

TEST(Small_RunRecord, ASiteIsLookedUpById)
{
    const RunRecord record = small_record();

    ASSERT_NE(record.site(1U), nullptr);
    EXPECT_EQ(record.site(1U)->name, "forward");
}

TEST(Small_RunRecord, SiteZeroAndAnUnknownSiteGiveNull)
{
    const RunRecord record = small_record();

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

TEST(Small_RunRecord, ACopyIsEqual)
{
    const RunRecord first = full_record();
    const RunRecord second = first;

    EXPECT_EQ(first, second);
}

class Small_RunRecordField : public ::testing::TestWithParam<FieldChange>
{
};

TEST_P(Small_RunRecordField, EqualityReachesTheField)
{
    const RunRecord first = full_record();
    RunRecord second = first;
    GetParam().change(second);

    EXPECT_FALSE(first == second) << GetParam().field;
}

INSTANTIATE_TEST_SUITE_P(
    Small_Fields, Small_RunRecordField,
    ::testing::Values(
        FieldChange{"environment", [](RunRecord& record) { record.environment[0].value = "y"; }},
        FieldChange{"build", [](RunRecord& record) { record.build[0].value = "Debug"; }},
        FieldChange{"tags", [](RunRecord& record) { record.tags[0].value = "b"; }},
        FieldChange{"clock.source", [](RunRecord& record) { record.clock.source = "tsc"; }},
        FieldChange{"clock.nanoseconds_per_tick",
                    [](RunRecord& record) { record.clock.nanoseconds_per_tick = 4.0; }},
        FieldChange{"clock.read_cost_ns",
                    [](RunRecord& record) { record.clock.read_cost_ns = 4.0; }},
        FieldChange{"names", [](RunRecord& record) { record.names[2] = "other"; }},
        FieldChange{"sites", [](RunRecord& record) { record.sites[1].group = "Host"; }},
        FieldChange{"tracks", [](RunRecord& record) { record.tracks[1] = "queue 1"; }},
        FieldChange{"threads", [](RunRecord& record) { record.threads[0].dropped = 9U; }},
        FieldChange{"periods", [](RunRecord& record) { record.periods[0].index = 9U; }},
        FieldChange{"drops.outside_periods",
                    [](RunRecord& record) { record.drops.outside_periods = 9U; }},
        FieldChange{"drops.open_at_stop",
                    [](RunRecord& record) { record.drops.open_at_stop = 9U; }},
        FieldChange{"drops.spans_dropped",
                    [](RunRecord& record) { record.drops.spans_dropped = 9U; }}));
