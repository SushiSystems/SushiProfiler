/**************************************************************************/
/* dispersion.hpp                                                         */
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
 * @file dispersion.hpp
 * @brief Declares the mean, the coefficient of variation and the jitter of samples.
 * @author Mustafa Garip
 * @see modules/record/statistics/README.md
 */

#pragma once

#include <SushiProfiler/version/export.hpp>

#include <vector>

namespace SushiProfiler
{
    /// @brief Returns the arithmetic mean of @p samples, or 0 for none.
    [[nodiscard]] SUSHIPROFILER_API double mean(const std::vector<double>& samples) noexcept;

    /**
     * @brief Returns the sample standard deviation over the mean.
     * @return 0 for fewer than two samples or a mean of 0.
     */
    [[nodiscard]] SUSHIPROFILER_API double coefficient_of_variation(
        const std::vector<double>& samples) noexcept;

    /**
     * @brief Returns the median absolute difference between successive samples of @p series.
     *
     * Unlike the standard deviation it ignores a slow drift and measures period-to-period noise.
     * @return 0 for fewer than two samples.
     */
    [[nodiscard]] SUSHIPROFILER_API double jitter(const std::vector<double>& series);
} // namespace SushiProfiler
