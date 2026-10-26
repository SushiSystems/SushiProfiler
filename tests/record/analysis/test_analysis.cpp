/**************************************************************************/
/* test_analysis.cpp                                                      */
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
 * @file test_analysis.cpp
 * @brief Tests the analysis options' defaults and that an analysis runs through its interface.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/analysis.hpp>

#include <gtest/gtest.h>

#include <memory>

using SushiProfiler::Analysis;
using SushiProfiler::AnalysisOptions;
using SushiProfiler::Cell;
using SushiProfiler::RunRecord;
using SushiProfiler::Table;

namespace
{
    /// @brief Reports how many periods a record holds.
    class PeriodCount final : public Analysis
    {
        public:
            std::string_view name() const noexcept override
            {
                return "period_count";
            }

            Table run(const RunRecord& record, const AnalysisOptions& options) const override
            {
                Table table;
                table.name = std::string(name());
                table.add_column("periods");
                table.rows.push_back({Cell::whole(static_cast<std::int64_t>(
                    record.periods.size() + options.top * 0U))});
                return table;
            }
    };
} // namespace

TEST(Small_Analysis, OptionsDefaultToTheDesignedValues)
{
    const AnalysisOptions options;

    EXPECT_EQ(options.top, 20U);
    EXPECT_EQ(options.short_span_ns, 10'000U);
    EXPECT_EQ(options.launch_floor_ns, 0U);
    EXPECT_EQ(options.bootstrap.resamples, 2000U);
    EXPECT_EQ(options.baseline, nullptr);
}

TEST(Small_Analysis, RunsThroughTheInterface)
{
    const std::unique_ptr<Analysis> analysis = std::make_unique<PeriodCount>();
    RunRecord record;
    record.periods.resize(3U);
    const Table table = analysis->run(record, {});

    EXPECT_EQ(table.name, "period_count");
    EXPECT_EQ(table.rows[0][0], Cell::whole(3));
}
