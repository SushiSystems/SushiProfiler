/**************************************************************************/
/* test_short_spans_analysis.cpp                                          */
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
 * @file test_short_spans_analysis.cpp
 * @brief Tests the count of spans under the bound and their cost at the launch floor.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/short_spans_analysis.hpp>

#include <gtest/gtest.h>

#include "analysis_record.hpp"

using SushiProfiler::AnalysisOptions;
using SushiProfiler::Table;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::span_record;
using SushiProfilerTests::whole;

TEST(Small_ShortSpansAnalysis, CountsTheSpansUnderTheBound)
{
    AnalysisOptions options;
    options.short_span_ns = 10U;
    const Table table = SushiProfiler::ShortSpansAnalysis().run(span_record(), options);

    ASSERT_EQ(table.rows.size(), 1U);
    expect_row(table.rows[0], {real(10.0), whole(5), whole(2), real(0.4), real(13.0),
                               real(13.0 / 1213.0), real(0.0), real(0.0)});
    EXPECT_EQ(table.notes.size(), 1U);
}

TEST(Small_ShortSpansAnalysis, PricesShortSpansAtTheLaunchFloor)
{
    AnalysisOptions options;
    options.short_span_ns = 10U;
    options.launch_floor_ns = 20U;
    const Table table = SushiProfiler::ShortSpansAnalysis().run(span_record(), options);

    expect_row(table.rows[0], {real(10.0), whole(5), whole(2), real(0.4), real(13.0),
                               real(13.0 / 1213.0), real(20.0), real(40.0)});
    EXPECT_TRUE(table.notes.empty());
}
