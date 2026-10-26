/**************************************************************************/
/* test_top_spans_analysis.cpp                                            */
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
 * @file test_top_spans_analysis.cpp
 * @brief Tests span groups by name and attribute, ranked by total time and cut at the top.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/top_spans_analysis.hpp>

#include <gtest/gtest.h>

#include "analysis_record.hpp"

using SushiProfiler::AnalysisOptions;
using SushiProfiler::Table;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::span_record;
using SushiProfilerTests::text;
using SushiProfilerTests::whole;

TEST(Small_TopSpansAnalysis, RanksSpansByNameAndTotalTime)
{
    const Table table = SushiProfiler::TopSpansAnalysis().run(span_record(), {});

    ASSERT_EQ(table.rows.size(), 2U);
    expect_row(table.rows[0], {text("gemm"), whole(3), real(1200.0), real(300.0),
                               real(1200.0 / 1213.0)});
    expect_row(table.rows[1], {text("copy"), whole(2), real(13.0), real(6.5), real(13.0 / 1213.0)});
    EXPECT_TRUE(table.notes.empty());
}

TEST(Small_TopSpansAnalysis, GroupsByAnAttributeAndKeepsTheTop)
{
    AnalysisOptions options;
    options.group_by = {"batch"};
    options.top = 2U;
    const Table table = SushiProfiler::TopSpansAnalysis().run(span_record(), options);

    ASSERT_EQ(table.columns[1].name, "batch");
    ASSERT_EQ(table.rows.size(), 2U);
    expect_row(table.rows[0], {text("gemm"), text("4"), whole(2), real(900.0), real(450.0),
                               real(900.0 / 1213.0)});
    expect_row(table.rows[1], {text("gemm"), text("8"), whole(1), real(300.0), real(300.0),
                               real(300.0 / 1213.0)});
    EXPECT_EQ(table.notes, std::vector<std::string>{"1 group is not shown."});
}
