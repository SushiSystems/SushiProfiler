/**************************************************************************/
/* bootstrap.cpp                                                          */
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
 * @file bootstrap.cpp
 * @brief Resamples with a portable generator and takes percentiles of the resampled medians.
 * @author Mustafa Garip
 * @see modules/record/statistics/README.md
 */

#include <SushiProfiler/statistics/bootstrap.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>

#include <SushiProfiler/statistics/sorted_samples.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Draws 64-bit values by SplitMix64.
        class SplitMix64 final
        {
            public:
                /// @brief Starts the sequence at @p seed.
                explicit SplitMix64(std::uint64_t seed) noexcept : state_(seed)
                {

                }

                /// @brief Returns the next value.
                std::uint64_t next() noexcept
                {
                    state_ += 0x9E3779B97F4A7C15ULL;
                    std::uint64_t mixed = state_;
                    mixed = (mixed ^ (mixed >> 30U)) * 0xBF58476D1CE4E5B9ULL;
                    mixed = (mixed ^ (mixed >> 27U)) * 0x94D049BB133111EBULL;
                    return mixed ^ (mixed >> 31U);
                }

                /// @brief Returns an index below @p count, from the next value's high 32 bits.
                std::size_t index_below(std::size_t count) noexcept
                {
                    const std::uint64_t high = next() >> 32U;
                    return static_cast<std::size_t>((high * count) >> 32U);
                }

            private:
                /// @brief The generator's state.
                std::uint64_t state_;
        };

        /// @brief Returns the median of @p values, reordering them.
        double median_in_place(std::vector<double>& values)
        {
            const std::size_t middle = values.size() / 2U;
            std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(middle),
                             values.end());
            const double upper = values[middle];
            if (values.size() % 2U != 0U)
                return upper;
            const auto middle_at = values.begin() + static_cast<std::ptrdiff_t>(middle);
            const double lower = *std::max_element(values.begin(), middle_at);
            return lower + 0.5 * (upper - lower);
        }
    } // namespace

    Interval bootstrap_median_interval(const std::vector<double>& samples,
                                       const BootstrapSettings& settings)
    {
        SplitMix64 generator(settings.seed);
        std::vector<double> draw(samples.size());
        std::vector<double> medians;
        medians.reserve(settings.resamples);
        for (std::uint32_t resample = 0U; resample < settings.resamples; ++resample)
        {
            for (double& value : draw)
                value = samples[generator.index_below(samples.size())];
            medians.push_back(median_in_place(draw));
        }
        const SortedSamples sorted(std::move(medians));
        const double tail = (1.0 - settings.confidence) * 50.0;
        return Interval{sorted.percentile(tail), sorted.percentile(100.0 - tail)};
    }
} // namespace SushiProfiler
