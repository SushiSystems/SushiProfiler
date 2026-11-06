/**************************************************************************/
/* bootstrap.hpp                                                          */
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
 * @file bootstrap.hpp
 * @brief Declares the bootstrap confidence interval of a median, reproducible from its seed.
 * @author Mustafa Garip
 * @see modules/record/statistics/README.md
 */

#pragma once

#include <SushiProfiler/version/export.hpp>

#include <cstdint>
#include <vector>

namespace SushiProfiler
{
    /// @brief A closed interval of values.
    struct Interval
    {
        double low = 0.0;  /**< The lower end. */
        double high = 0.0; /**< The upper end. */
    };

    /// @brief How a bootstrap resamples.
    struct BootstrapSettings
    {
        std::uint32_t resamples = 2000U; /**< How many resamples to draw. */
        double confidence = 0.95;        /**< The interval's coverage, in (0, 1). */
        std::uint64_t seed = 0x5EEDU;    /**< The generator's seed. */
    };

    /**
     * @brief Returns the percentile bootstrap interval of the median of @p samples.
     *
     * Draws come from SplitMix64 mapped to an index by a 32-bit multiply and shift, so the same
     * samples and settings give the same interval on every compiler and platform.
     * @pre @p samples is not empty and holds no NaN; `settings.resamples` is at least 1.
     */
    [[nodiscard]] SUSHIPROFILER_API Interval bootstrap_median_interval(
        const std::vector<double>& samples, const BootstrapSettings& settings = {});
} // namespace SushiProfiler
