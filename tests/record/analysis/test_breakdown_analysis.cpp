/**************************************************************************/
/* test_breakdown_analysis.cpp                                            */
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
 * @file test_breakdown_analysis.cpp
 * @brief Tests the breakdown rows: self time per group, then the uncovered time.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/breakdown_analysis.hpp>

#include <gtest/gtest.h>

#include <cmath>

#include "analysis_record.hpp"

using SushiProfiler::Table;
using SushiProfilerTests::analysis_record;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::text;
using SushiProfilerTests::whole;

TEST(Small_BreakdownAnalysis, ReportsSelfTimePerGroupThenTheUncoveredTime)
{
    const Table table = SushiProfiler::BreakdownAnalysis().run(analysis_record(), {});

    ASSERT_EQ(table.rows.size(), 3U);
    expect_row(table.rows[0], {text("step"), text("Model"), whole(3), real(70.0), real(60.0),
                               real(80.0), real(60.0), real(79.0), real(10.0 / 70.0),
                               real(210.0 / 330.0)});
    expect_row(table.rows[1], {text("step"), text("Data"), whole(3), real(30.0), real(20.0),
                               real(30.0), real(20.0), real(30.0),
                               real(std::sqrt(100.0 / 3.0) / (80.0 / 3.0)), real(80.0 / 330.0)});
    expect_row(table.rows[2], {text("step"), text("(uncovered)"), whole(3), real(10.0),
                               real(10.0), real(20.0), real(10.0), real(19.0),
                               real(std::sqrt(100.0 / 3.0) / (40.0 / 3.0)), real(40.0 / 330.0)});
}
