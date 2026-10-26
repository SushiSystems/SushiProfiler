/**************************************************************************/
/* analysis_record.hpp                                                    */
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
 * @file analysis_record.hpp
 * @brief Declares the hand-built record the analysis tests share, and a row matcher.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#pragma once

#include <SushiProfiler/analysis/table.hpp>
#include <SushiProfiler/record/run_record.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace SushiProfilerTests
{
    /**
     * @brief Returns the record the analysis tests read.
     *
     * Timeline `step` on thread 0 has a warm-up period and three active periods of 100, 120 and
     * 110 ns, each with a 105 ns budget, zones in groups `Model` and `Data`, two counters, a
     * value and a flow. tests/record/analysis/README.md lists every number.
     */
    [[nodiscard]] SushiProfiler::RunRecord analysis_record();

    /**
     * @brief Returns the record the span analysis tests read.
     *
     * Track `queue 0` runs `gemm` and `copy` spans in a warm-up period and two active periods of
     * 1000 ns each; tests/record/analysis/README.md lists every span.
     */
    [[nodiscard]] SushiProfiler::RunRecord span_record();

    /// @brief Returns a text cell.
    [[nodiscard]] SushiProfiler::Cell text(const char* value);

    /// @brief Returns a whole-number cell.
    [[nodiscard]] SushiProfiler::Cell whole(std::int64_t value);

    /// @brief Returns a real cell.
    [[nodiscard]] SushiProfiler::Cell real(double value);

    /// @brief Expects @p row to equal @p expected, reals within 1e-9 relative.
    void expect_row(const SushiProfiler::Row& row, const SushiProfiler::Row& expected);
} // namespace SushiProfilerTests
