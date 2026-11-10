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
 * @file test_budget_analysis.cpp
 * @brief Tests the budget row: used fraction, periods over budget and the worst period.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/budget_analysis.hpp>

#include <gtest/gtest.h>

#include "analysis_record.hpp"

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
    const Table table =
        SushiProfiler::BudgetAnalysis().run(SushiProfilerTests::unbudgeted_analysis_record(), {});

    EXPECT_TRUE(table.rows.empty());
}
