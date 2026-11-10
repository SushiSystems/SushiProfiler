/**************************************************************************/
/* test_period_record.cpp                                                 */
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
 * @file test_period_record.cpp
 * @brief Tests a period's duration and that equality reaches its trees, tags and spans.
 * @author Mustafa Garip
 * @see tests/record/record/README.md
 */

#include <SushiProfiler/record/period_record.hpp>

#include <gtest/gtest.h>

using SushiProfiler::PeriodRecord;
using SushiProfiler::SampleSeries;
using SushiProfiler::ZoneNode;
using SushiProfiler::ZoneRun;
using SushiProfiler::ZoneTree;

namespace
{
    /// @brief Returns a period whose every field differs from the default.
    PeriodRecord full_period()
    {
        PeriodRecord period;
        period.index = 1U;
        period.timeline = 2U;
        period.thread = 3U;
        period.begin_ns = 4U;
        period.end_ns = 5U;
        period.budget_ns = 6U;
        period.tags.push_back({1U, 2U});
        period.facts.push_back({"fact", "value"});
        ZoneTree tree;
        tree.nodes.push_back(ZoneNode{1U, 0U, -1, 10U, 10U, 1U});
        period.zones.push_back(tree);
        period.runs.push_back(ZoneRun{1U, 2U, 3U, 4U, 5U, 6U, 7U});
        period.spans.append({});
        SampleSeries series;
        series.site = 3U;
        series.time_ns = {1U};
        series.values = {1.0};
        period.samples.push_back(series);
        return period;
    }

    /// @brief A field of a period and a change to it alone.
    struct FieldChange
    {
        const char* field;
        void (*change)(PeriodRecord&);
    };
} // namespace

TEST(Small_PeriodRecord, DurationIsEndLessBegin)
{
    PeriodRecord period;
    period.begin_ns = 1'000U;
    period.end_ns = 5'000'001'000U;

    EXPECT_EQ(period.duration_ns(), 5'000'000'000U);
}

TEST(Small_PeriodRecord, ACopyIsEqual)
{
    const PeriodRecord first = full_period();
    const PeriodRecord second = first;

    EXPECT_EQ(first, second);
}

class Small_PeriodRecordField : public ::testing::TestWithParam<FieldChange>
{
};

TEST_P(Small_PeriodRecordField, EqualityReachesTheField)
{
    const PeriodRecord first = full_period();
    PeriodRecord second = first;
    GetParam().change(second);

    EXPECT_FALSE(first == second) << GetParam().field;
}

INSTANTIATE_TEST_SUITE_P(
    Small_Fields, Small_PeriodRecordField,
    ::testing::Values(
        FieldChange{"index", [](PeriodRecord& period) { period.index = 9U; }},
        FieldChange{"timeline", [](PeriodRecord& period) { period.timeline = 9U; }},
        FieldChange{"thread", [](PeriodRecord& period) { period.thread = 9U; }},
        FieldChange{"begin_ns", [](PeriodRecord& period) { period.begin_ns = 9U; }},
        FieldChange{"end_ns", [](PeriodRecord& period) { period.end_ns = 9U; }},
        FieldChange{"warmup", [](PeriodRecord& period) { period.warmup = true; }},
        FieldChange{"budget_ns", [](PeriodRecord& period) { period.budget_ns = 9U; }},
        FieldChange{"tags", [](PeriodRecord& period) { period.tags[0].value = 3U; }},
        FieldChange{"facts", [](PeriodRecord& period) { period.facts[0].value = "other"; }},
        FieldChange{"zones", [](PeriodRecord& period) { period.zones[0].nodes[0].calls = 2U; }},
        FieldChange{"runs.thread", [](PeriodRecord& period) { period.runs[0].thread = 9U; }},
        FieldChange{"runs.lane", [](PeriodRecord& period) { period.runs[0].lane = 9U; }},
        FieldChange{"runs.site", [](PeriodRecord& period) { period.runs[0].site = 9U; }},
        FieldChange{"runs.name", [](PeriodRecord& period) { period.runs[0].name = 9U; }},
        FieldChange{"runs.begin_ns", [](PeriodRecord& period) { period.runs[0].begin_ns = 0U; }},
        FieldChange{"runs.end_ns", [](PeriodRecord& period) { period.runs[0].end_ns = 0U; }},
        FieldChange{"runs.depth", [](PeriodRecord& period) { period.runs[0].depth = 8U; }},
        FieldChange{"runs", [](PeriodRecord& period) { period.runs.clear(); }},
        FieldChange{"spans", [](PeriodRecord& period) { period.spans.append({}); }},
        FieldChange{"samples", [](PeriodRecord& period) { period.samples[0].site = 9U; }}));
