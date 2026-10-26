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

TEST(Small_OverheadAnalysis, CountsEventsAndDropsAndNotesAMissingBaseline)
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
    EXPECT_EQ(table.notes.size(), 1U);
}

TEST(Small_OverheadAnalysis, MeasuresTheCostAgainstABaseline)
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
    AnalysisOptions options;
    options.baseline = &baseline;
    const Table table = SushiProfiler::OverheadAnalysis().run(analysis_record(), options);

    ASSERT_EQ(table.rows.size(), 11U);
    expect_row(table.rows[8], {text("baseline_median_ns:step"), real(100.0)});
    expect_row(table.rows[9], {text("median_ns:step"), real(110.0)});
    expect_row(table.rows[10], {text("cost:step"), real(0.1)});
    EXPECT_TRUE(table.notes.empty());
}
