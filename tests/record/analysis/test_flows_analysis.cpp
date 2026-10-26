/**************************************************************************/
/* test_flows_analysis.cpp                                                */
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
 * @file test_flows_analysis.cpp
 * @brief Tests flow latencies from begin to end and between steps.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/flows_analysis.hpp>

#include <gtest/gtest.h>

#include <cmath>

#include "analysis_record.hpp"

using SushiProfiler::Table;
using SushiProfilerTests::analysis_record;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::text;
using SushiProfilerTests::whole;

TEST(Small_FlowsAnalysis, MeasuresTotalAndStepLatencyAcrossPeriods)
{
    const Table table = SushiProfiler::FlowsAnalysis().run(analysis_record(), {});

    ASSERT_EQ(table.rows.size(), 2U);
    expect_row(table.rows[0], {text("item"), text("total"), whole(2), real(95.0), real(90.0),
                               real(100.0), real(90.0), real(99.5),
                               real(std::sqrt(50.0) / 95.0)});
    expect_row(table.rows[1], {text("item"), text("step"), whole(4), real(50.0), real(10.0),
                               real(80.0), real(10.0), real(80.0),
                               real(std::sqrt(1425.0) / 47.5)});
}
