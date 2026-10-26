/**************************************************************************/
/* test_analysis_runner.cpp                                               */
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
 * @file test_analysis_runner.cpp
 * @brief Tests that the runner keeps the order asked and refuses an unknown name up front.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/analysis_runner.hpp>

#include <gtest/gtest.h>

#include "analysis_record.hpp"

using SushiProfiler::AnalysisCatalog;
using SushiProfiler::AnalysisRun;
using SushiProfiler::AnalysisRunner;

namespace
{
    /// @brief Returns a catalog holding the built-in analyses.
    AnalysisCatalog builtins()
    {
        AnalysisCatalog catalog;
        SushiProfiler::register_builtin_analyses(catalog);
        return catalog;
    }
} // namespace

TEST(Small_AnalysisRunner, RunsTheNamedAnalysesInTheOrderAsked)
{
    const AnalysisCatalog catalog = builtins();
    const AnalysisRun run = AnalysisRunner(catalog).run(SushiProfilerTests::analysis_record(),
                                                        {"flows", "budget"}, {});

    ASSERT_TRUE(run.ok()) << run.error;
    ASSERT_EQ(run.tables.size(), 2U);
    EXPECT_EQ(run.tables[0].name, "flows");
    EXPECT_EQ(run.tables[1].name, "budget");
}

TEST(Small_AnalysisRunner, AnUnknownNameRefusesTheWholeRun)
{
    const AnalysisCatalog catalog = builtins();
    const AnalysisRun run = AnalysisRunner(catalog).run(SushiProfilerTests::analysis_record(),
                                                        {"flows", "report"}, {});

    EXPECT_FALSE(run.ok());
    EXPECT_TRUE(run.tables.empty());
    EXPECT_EQ(run.error, "unknown analysis \"report\"; known: breakdown budget counters "
                         "device_idle distribution flows overhead roofline short_spans top_spans");
}
