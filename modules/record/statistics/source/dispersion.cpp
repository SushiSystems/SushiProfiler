/**************************************************************************/
/* dispersion.cpp                                                         */
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
 * @file dispersion.cpp
 * @brief Computes the mean, the coefficient of variation and the successive-difference jitter.
 * @author Mustafa Garip
 * @see modules/record/statistics/README.md
 */

#include <SushiProfiler/statistics/dispersion.hpp>

#include <cmath>
#include <cstddef>
#include <utility>

#include <SushiProfiler/statistics/sorted_samples.hpp>

namespace SushiProfiler
{
    double mean(const std::vector<double>& samples) noexcept
    {
        if (samples.empty())
            return 0.0;
        double sum = 0.0;
        for (const double sample : samples)
            sum += sample;
        return sum / static_cast<double>(samples.size());
    }

    double coefficient_of_variation(const std::vector<double>& samples) noexcept
    {
        const double average = mean(samples);
        if (samples.size() < 2U || average == 0.0)
            return 0.0;
        double squares = 0.0;
        for (const double sample : samples)
            squares += (sample - average) * (sample - average);
        const double variance = squares / static_cast<double>(samples.size() - 1U);
        return std::sqrt(variance) / std::fabs(average);
    }

    double jitter(const std::vector<double>& series)
    {
        if (series.size() < 2U)
            return 0.0;
        std::vector<double> steps;
        steps.reserve(series.size() - 1U);
        for (std::size_t index = 1U; index < series.size(); ++index)
            steps.push_back(std::fabs(series[index] - series[index - 1U]));
        return SortedSamples(std::move(steps)).median();
    }
} // namespace SushiProfiler
