/**************************************************************************/
/* test_counters_analysis.cpp                                             */
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
 * @file test_counters_analysis.cpp
 * @brief Tests counters folded per period by their operation.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/counters_analysis.hpp>

#include <gtest/gtest.h>

#include <cmath>

#include "analysis_record.hpp"

using SushiProfiler::Table;
using SushiProfilerTests::analysis_record;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::text;
using SushiProfilerTests::whole;

TEST(Small_CountersAnalysis, FoldsEachCounterByItsOperation)
{
    const Table table = SushiProfiler::CountersAnalysis().run(analysis_record(), {});

    ASSERT_EQ(table.rows.size(), 2U);
    expect_row(table.rows[0], {text("bytes"), text("sum"), text("B"), whole(2), real(175.0),
                               real(150.0), real(200.0), real(150.0), real(197.5),
                               real(std::sqrt(1250.0) / 175.0)});
    expect_row(table.rows[1], {text("peak"), text("max"), text("B"), whole(3), real(7.0),
                               real(4.0), real(9.0), real(4.0), real(8.8),
                               real(std::sqrt(57.0 / 9.0) / (20.0 / 3.0))});
}
