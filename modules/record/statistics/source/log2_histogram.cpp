/**************************************************************************/
/* log2_histogram.cpp                                                     */
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
 * @file log2_histogram.cpp
 * @brief Bins samples by the exponent frexp gives, which is exact at every power of two.
 * @author Mustafa Garip
 * @see modules/record/statistics/README.md
 */

#include <SushiProfiler/statistics/log2_histogram.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns floor(log2(@p value)) for a positive finite value.
        int floor_log2(double value) noexcept
        {
            int exponent = 0;
            static_cast<void>(std::frexp(value, &exponent));
            return exponent - 1;
        }
    } // namespace

    bool Log2Histogram::operator==(const Log2Histogram& other) const
    {
        return nonpositive == other.nonpositive && first_exponent == other.first_exponent &&
               counts == other.counts;
    }

    Log2Histogram log2_histogram(const std::vector<double>& samples)
    {
        Log2Histogram histogram;
        bool any = false;
        int lowest = 0;
        int highest = 0;
        for (const double sample : samples)
        {
            if (sample <= 0.0)
                continue;
            const int exponent = floor_log2(sample);
            lowest = any ? std::min(lowest, exponent) : exponent;
            highest = any ? std::max(highest, exponent) : exponent;
            any = true;
        }
        if (any)
        {
            histogram.first_exponent = lowest;
            histogram.counts.assign(static_cast<std::size_t>(highest - lowest + 1), 0U);
        }
        for (const double sample : samples)
        {
            if (sample <= 0.0)
                ++histogram.nonpositive;
            else
                ++histogram.counts[static_cast<std::size_t>(floor_log2(sample) - lowest)];
        }
        return histogram;
    }
} // namespace SushiProfiler
