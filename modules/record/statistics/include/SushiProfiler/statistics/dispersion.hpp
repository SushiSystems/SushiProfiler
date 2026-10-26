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
 * @file dispersion.hpp
 * @brief Declares the mean, the coefficient of variation and the jitter of samples.
 * @author Mustafa Garip
 * @see modules/record/statistics/README.md
 */

#pragma once

#include <vector>

namespace SushiProfiler
{
    /// @brief Returns the arithmetic mean of @p samples, or 0 for none.
    [[nodiscard]] double mean(const std::vector<double>& samples) noexcept;

    /**
     * @brief Returns the sample standard deviation over the mean.
     * @return 0 for fewer than two samples or a mean of 0.
     */
    [[nodiscard]] double coefficient_of_variation(const std::vector<double>& samples) noexcept;

    /**
     * @brief Returns the median absolute difference between successive samples of @p series.
     *
     * Unlike the standard deviation it ignores a slow drift and measures period-to-period noise.
     * @return 0 for fewer than two samples.
     */
    [[nodiscard]] double jitter(const std::vector<double>& series);
} // namespace SushiProfiler
