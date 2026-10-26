/**************************************************************************/
/* test_sample_summary.cpp                                                */
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
 * @file test_sample_summary.cpp
 * @brief Tests the summary's numbers and the columns that hold them.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/sample_summary.hpp>

#include <gtest/gtest.h>

#include <cmath>

using SushiProfiler::Row;
using SushiProfiler::SampleSummary;
using SushiProfiler::Table;
using SushiProfiler::Unit;

TEST(Small_SampleSummary, SummarisesFixedSamples)
{
    const SampleSummary summary = SushiProfiler::summarise({20.0, 80.0, 80.0, 10.0}, {});

    EXPECT_EQ(summary.count, 4);
    EXPECT_EQ(summary.median, 50.0);
    EXPECT_EQ(summary.low, 10.0);
    EXPECT_EQ(summary.high, 80.0);
    EXPECT_EQ(summary.minimum, 10.0);
    EXPECT_EQ(summary.p95, 80.0);
    EXPECT_NEAR(summary.cv, std::sqrt(1425.0) / 47.5, 1e-12);
}

TEST(Small_SampleSummary, NoSamplesGiveZeros)
{
    const SampleSummary summary = SushiProfiler::summarise({}, {});

    EXPECT_EQ(summary.count, 0);
    EXPECT_EQ(summary.median, 0.0);
    EXPECT_EQ(summary.high, 0.0);
}

TEST(Small_SampleSummary, ColumnsAndCellsLineUp)
{
    Table table;
    SushiProfiler::add_summary_columns(table, Unit::Nanoseconds);
    Row row;
    SushiProfiler::append_summary(row, SampleSummary{});

    ASSERT_EQ(table.columns.size(), 7U);
    EXPECT_EQ(row.size(), table.columns.size());
    EXPECT_EQ(table.columns[0].name, "count");
    EXPECT_EQ(table.columns[1].unit, Unit::Nanoseconds);
    EXPECT_EQ(table.columns[6].name, "cv");
    EXPECT_EQ(table.columns[6].unit, Unit::Ratio);
}
