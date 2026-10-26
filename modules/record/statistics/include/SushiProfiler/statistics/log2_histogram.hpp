/**************************************************************************/
/* log2_histogram.hpp                                                     */
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
 * @file log2_histogram.hpp
 * @brief Declares a histogram with one bin per power of two.
 * @author Mustafa Garip
 * @see modules/record/statistics/README.md
 */

#pragma once

#include <cstdint>
#include <vector>

namespace SushiProfiler
{
    /// @brief Counts samples by power of two: bin i holds [2^(first + i), 2^(first + i + 1)).
    struct Log2Histogram
    {
        std::uint64_t nonpositive = 0U;    /**< Samples at or below 0, which have no bin. */
        int first_exponent = 0;            /**< The exponent of the first bin. */
        std::vector<std::uint64_t> counts; /**< One count per exponent, empty bins included. */

        /// @brief Returns whether every field is equal.
        [[nodiscard]] bool operator==(const Log2Histogram& other) const;
    };

    /// @brief Returns the histogram of @p samples; none may be NaN or infinite.
    [[nodiscard]] Log2Histogram log2_histogram(const std::vector<double>& samples);
} // namespace SushiProfiler
