/**************************************************************************/
/* sorted_samples.hpp                                                     */
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
 * @file sorted_samples.hpp
 * @brief Declares a sorted copy of samples that answers order statistics.
 * @author Mustafa Garip
 * @see modules/record/statistics/README.md
 */

#pragma once

#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Holds samples sorted once, so each order statistic costs a lookup.
     *
     * Percentiles use the type-7 rule: rank (n - 1) p / 100, interpolated linearly between the
     * two samples around it. Every query other than size and empty requires at least one sample.
     */
    class SUSHIPROFILER_API SortedSamples final
    {
        public:
            /// @brief Sorts @p samples; none may be NaN.
            explicit SortedSamples(std::vector<double> samples);

            /// @brief Returns the number of samples.
            [[nodiscard]] std::size_t size() const noexcept;

            /// @brief Returns whether there are no samples.
            [[nodiscard]] bool empty() const noexcept;

            /// @brief Returns the smallest sample.
            [[nodiscard]] double minimum() const noexcept;

            /// @brief Returns the largest sample.
            [[nodiscard]] double maximum() const noexcept;

            /// @brief Returns the median, the 50th percentile.
            [[nodiscard]] double median() const noexcept;

            /// @brief Returns the type-7 percentile @p p.
            /// @pre @p p is in [0, 100].
            [[nodiscard]] double percentile(double p) const noexcept;

            /// @brief Returns the samples in ascending order.
            [[nodiscard]] const std::vector<double>& values() const noexcept;

        private:
            /// @brief The samples, ascending.
            std::vector<double> values_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
