/**************************************************************************/
/* test_analysis_catalog.cpp                                              */
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
 * @file test_analysis_catalog.cpp
 * @brief Tests that the catalog makes analyses by name and refuses a taken name.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/analysis_catalog.hpp>

#include <gtest/gtest.h>

#include <SushiProfiler/analysis/breakdown_analysis.hpp>

using SushiProfiler::Analysis;
using SushiProfiler::AnalysisCatalog;

namespace
{
    /// @brief Makes a breakdown analysis.
    std::unique_ptr<Analysis> make_breakdown()
    {
        return std::make_unique<SushiProfiler::BreakdownAnalysis>();
    }
} // namespace

TEST(Small_AnalysisCatalog, MakesWhatItKnowsAndNothingElse)
{
    AnalysisCatalog catalog;
    ASSERT_TRUE(catalog.add("breakdown", &make_breakdown));

    const std::unique_ptr<Analysis> made = catalog.make("breakdown");
    ASSERT_NE(made, nullptr);
    EXPECT_EQ(made->name(), "breakdown");
    EXPECT_EQ(catalog.make("missing"), nullptr);
    EXPECT_TRUE(catalog.knows("breakdown"));
    EXPECT_FALSE(catalog.knows("missing"));
}

TEST(Small_AnalysisCatalog, RefusesATakenName)
{
    AnalysisCatalog catalog;
    ASSERT_TRUE(catalog.add("breakdown", &make_breakdown));

    EXPECT_FALSE(catalog.add("breakdown", &make_breakdown));
}

TEST(Small_AnalysisCatalog, TheBuiltinsAreListedInOrder)
{
    AnalysisCatalog catalog;
    SushiProfiler::register_builtin_analyses(catalog);

    const std::vector<std::string> expected = {
        "breakdown", "budget",   "counters", "device_idle", "distribution",
        "flows",     "overhead", "roofline", "short_spans", "top_spans"};
    EXPECT_EQ(catalog.names(), expected);
}
