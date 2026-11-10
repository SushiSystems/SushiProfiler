/**************************************************************************/
/* test_overhead_analysis.cpp                                             */
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
 * @file test_overhead_analysis.cpp
 * @brief Tests the overhead rows, with and without a baseline record.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/overhead_analysis.hpp>

#include <gtest/gtest.h>

#include "analysis_record.hpp"

using SushiProfiler::AnalysisOptions;
using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::Table;
using SushiProfilerTests::analysis_record;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::text;
using SushiProfilerTests::whole;

namespace
{
    /// @brief Returns a run without profiling whose `step` periods last 90, 100 and 110 ns.
    RunRecord baseline_record()
    {
        RunRecord baseline;
        baseline.names = {"", "step"};
        for (const std::uint64_t length : {90U, 100U, 110U})
        {
            PeriodRecord period;
            period.timeline = 1U;
            period.end_ns = length;
            baseline.periods.push_back(period);
        }
        return baseline;
    }
} // namespace

TEST(Small_OverheadAnalysis, CountsEventsAndDrops)
{
    const Table table = SushiProfiler::OverheadAnalysis().run(analysis_record(), {});

    ASSERT_EQ(table.rows.size(), 9U);
    expect_row(table.rows[0], {text("events"), real(33.0)});
    expect_row(table.rows[1], {text("bytes"), real(33.0 * 24.0)});
    expect_row(table.rows[2], {text("dropped"), real(2.0)});
    expect_row(table.rows[3], {text("late"), real(1.0)});
    expect_row(table.rows[4], {text("unmatched"), real(4.0)});
    expect_row(table.rows[5], {text("outside_periods"), real(1.0)});
    expect_row(table.rows[6], {text("open_at_stop"), real(2.0)});
    expect_row(table.rows[7], {text("spans_dropped"), real(3.0)});
    expect_row(table.rows[8], {text("clock_read_ns"), real(6.5)});
}

TEST(Small_OverheadAnalysis, NotesAMissingBaseline)
{
    const Table table = SushiProfiler::OverheadAnalysis().run(analysis_record(), {});

    ASSERT_EQ(table.notes.size(), 1U);
    EXPECT_EQ(table.notes[0], "No baseline record was given, so the cost is not measured.");
}

TEST(Small_OverheadAnalysis, MeasuresTheCostAgainstABaseline)
{
    const RunRecord baseline = baseline_record();
    AnalysisOptions options;
    options.baseline = &baseline;
    const Table table = SushiProfiler::OverheadAnalysis().run(analysis_record(), options);

    ASSERT_EQ(table.rows.size(), 12U);
    expect_row(table.rows[9], {text("baseline_median_ns:step"), real(100.0)});
    expect_row(table.rows[10], {text("median_ns:step"), real(110.0)});
    expect_row(table.rows[11], {text("cost:step"), real(0.1)});
    EXPECT_TRUE(table.notes.empty());
}
