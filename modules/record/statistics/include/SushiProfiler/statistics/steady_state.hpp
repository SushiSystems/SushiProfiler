/**************************************************************************/
/* steady_state.hpp                                                       */
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
 * @file steady_state.hpp
 * @brief Declares the MSER-5 rule that finds where a series stops warming up.
 * @author Mustafa Garip
 * @see modules/record/statistics/README.md
 */

#pragma once

#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <vector>

namespace SushiProfiler
{
    /// @brief The number of samples MSER-5 averages into one batch.
    inline constexpr std::size_t K_MSER_BATCH = 5U;

    /**
     * @brief Returns how many leading samples of @p series to drop as warm-up, by MSER-5.
     *
     * The series is averaged in batches of five, and the cut d over the first half of the batches
     * minimises the variance of the remaining batch means over (m - d)^2; the earliest cut wins a
     * tie. A trailing partial batch is ignored.
     * @return A multiple of five; 0 for fewer than two batches.
     */
    [[nodiscard]] SUSHIPROFILER_API std::size_t mser5(const std::vector<double>& series);
} // namespace SushiProfiler
