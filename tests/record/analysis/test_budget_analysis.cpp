/**************************************************************************/
/* test_budget_analysis.cpp                                               */
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
 * @file test_budget_analysis.cpp
 * @brief Tests the budget row: used fraction, periods over budget and the worst period.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/budget_analysis.hpp>

#include <gtest/gtest.h>

#include "analysis_record.hpp"

using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::Table;
using SushiProfilerTests::analysis_record;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::text;
using SushiProfilerTests::whole;

TEST(Small_BudgetAnalysis, FindsThePeriodsOverBudgetAndTheWorst)
{
    const Table table = SushiProfiler::BudgetAnalysis().run(analysis_record(), {});

    ASSERT_EQ(table.rows.size(), 1U);
    expect_row(table.rows[0], {text("step"), whole(3), real(110.0 / 105.0), whole(2), whole(2),
                               real(120.0 / 105.0), text("forward")});
}

TEST(Small_BudgetAnalysis, ATimelineWithoutABudgetHasNoRow)
{
    RunRecord record = analysis_record();
    for (PeriodRecord& period : record.periods)
        period.budget_ns = 0U;

    EXPECT_TRUE(SushiProfiler::BudgetAnalysis().run(record, {}).rows.empty());
}
