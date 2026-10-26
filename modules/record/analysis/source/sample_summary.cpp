/**************************************************************************/
/* sample_summary.cpp                                                     */
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
 * @file sample_summary.cpp
 * @brief Summarises samples with the statistics module and lays the summary out as cells.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/sample_summary.hpp>

#include <SushiProfiler/statistics/dispersion.hpp>
#include <SushiProfiler/statistics/sorted_samples.hpp>

namespace SushiProfiler
{
    SampleSummary summarise(const std::vector<double>& samples, const BootstrapSettings& bootstrap)
    {
        SampleSummary summary;
        if (samples.empty())
            return summary;
        const SortedSamples sorted(samples);
        const Interval interval = bootstrap_median_interval(samples, bootstrap);
        summary.count = static_cast<std::int64_t>(samples.size());
        summary.median = sorted.median();
        summary.low = interval.low;
        summary.high = interval.high;
        summary.minimum = sorted.minimum();
        summary.p95 = sorted.percentile(95.0);
        summary.cv = coefficient_of_variation(samples);
        return summary;
    }

    void add_summary_columns(Table& table, Unit unit)
    {
        table.add_column("count", Unit::Count);
        table.add_column("median", unit);
        table.add_column("low", unit);
        table.add_column("high", unit);
        table.add_column("min", unit);
        table.add_column("p95", unit);
        table.add_column("cv", Unit::Ratio);
    }

    void append_summary(Row& row, const SampleSummary& summary)
    {
        row.push_back(Cell::whole(summary.count));
        row.push_back(Cell::real(summary.median));
        row.push_back(Cell::real(summary.low));
        row.push_back(Cell::real(summary.high));
        row.push_back(Cell::real(summary.minimum));
        row.push_back(Cell::real(summary.p95));
        row.push_back(Cell::real(summary.cv));
    }
} // namespace SushiProfiler
