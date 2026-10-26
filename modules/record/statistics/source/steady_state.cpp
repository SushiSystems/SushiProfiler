/**************************************************************************/
/* steady_state.cpp                                                       */
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
 * @file steady_state.cpp
 * @brief Averages a series in batches of five and picks the cut with the least scaled variance.
 * @author Mustafa Garip
 * @see modules/record/statistics/README.md
 */

#include <SushiProfiler/statistics/steady_state.hpp>

namespace SushiProfiler
{
    std::size_t mser5(const std::vector<double>& series)
    {
        const std::size_t batches = series.size() / K_MSER_BATCH;
        if (batches < 2U)
            return 0U;
        std::vector<double> means(batches, 0.0);
        for (std::size_t batch = 0U; batch < batches; ++batch)
        {
            double sum = 0.0;
            for (std::size_t offset = 0U; offset < K_MSER_BATCH; ++offset)
                sum += series[batch * K_MSER_BATCH + offset];
            means[batch] = sum / static_cast<double>(K_MSER_BATCH);
        }
        std::size_t best_cut = 0U;
        double best_score = 0.0;
        for (std::size_t cut = 0U; cut <= batches / 2U; ++cut)
        {
            const double kept = static_cast<double>(batches - cut);
            double sum = 0.0;
            for (std::size_t batch = cut; batch < batches; ++batch)
                sum += means[batch];
            const double average = sum / kept;
            double squares = 0.0;
            for (std::size_t batch = cut; batch < batches; ++batch)
                squares += (means[batch] - average) * (means[batch] - average);
            const double score = squares / (kept * kept);
            if (cut == 0U || score < best_score)
            {
                best_cut = cut;
                best_score = score;
            }
        }
        return best_cut * K_MSER_BATCH;
    }
} // namespace SushiProfiler
