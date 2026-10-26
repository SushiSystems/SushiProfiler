/**************************************************************************/
/* sample_summary.hpp                                                     */
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
 * @file sample_summary.hpp
 * @brief Declares the summary every timing row reports and the columns that hold it.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#pragma once

#include <SushiProfiler/analysis/table.hpp>
#include <SushiProfiler/statistics/bootstrap.hpp>

#include <cstdint>
#include <vector>

namespace SushiProfiler
{
    /// @brief Median, its bootstrap interval, minimum, p95 and coefficient of variation.
    struct SampleSummary
    {
        std::int64_t count = 0;   /**< How many samples. */
        double median = 0.0;      /**< The median. */
        double low = 0.0;         /**< The interval's lower end. */
        double high = 0.0;        /**< The interval's upper end. */
        double minimum = 0.0;     /**< The smallest sample. */
        double p95 = 0.0;         /**< The 95th percentile. */
        double cv = 0.0;          /**< The coefficient of variation. */
    };

    /// @brief Returns the summary of @p samples; every field but count is 0 for none.
    [[nodiscard]] SampleSummary summarise(const std::vector<double>& samples,
                                          const BootstrapSettings& bootstrap);

    /// @brief Appends the summary's columns: count, median, low, high, min, p95, cv.
    void add_summary_columns(Table& table, Unit unit);

    /// @brief Appends @p summary's cells to @p row, in the order of add_summary_columns.
    void append_summary(Row& row, const SampleSummary& summary);
} // namespace SushiProfiler
