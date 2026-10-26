/**************************************************************************/
/* sorted_samples.cpp                                                     */
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
 * @file sorted_samples.cpp
 * @brief Sorts samples once and interpolates type-7 percentiles between them.
 * @author Mustafa Garip
 * @see modules/record/statistics/README.md
 */

#include <SushiProfiler/statistics/sorted_samples.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace SushiProfiler
{
    SortedSamples::SortedSamples(std::vector<double> samples) : values_(std::move(samples))
    {
        std::sort(values_.begin(), values_.end());
    }

    std::size_t SortedSamples::size() const noexcept
    {
        return values_.size();
    }

    bool SortedSamples::empty() const noexcept
    {
        return values_.empty();
    }

    double SortedSamples::minimum() const noexcept
    {
        return values_.front();
    }

    double SortedSamples::maximum() const noexcept
    {
        return values_.back();
    }

    double SortedSamples::median() const noexcept
    {
        return percentile(50.0);
    }

    double SortedSamples::percentile(double p) const noexcept
    {
        const double rank = static_cast<double>(values_.size() - 1U) * p / 100.0;
        const double floor = std::floor(rank);
        const auto below = static_cast<std::size_t>(floor);
        if (below + 1U >= values_.size())
            return values_.back();
        return values_[below] + (rank - floor) * (values_[below + 1U] - values_[below]);
    }

    const std::vector<double>& SortedSamples::values() const noexcept
    {
        return values_;
    }
} // namespace SushiProfiler
