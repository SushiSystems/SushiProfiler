/**************************************************************************/
/* distribution_analysis.cpp                                              */
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
 * @file distribution_analysis.cpp
 * @brief Collects per-period times and samples per subject and reports their spread.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/distribution_analysis.hpp>

#include <string>
#include <utility>
#include <vector>

#include <SushiProfiler/analysis/subject_samples.hpp>
#include <SushiProfiler/statistics/dispersion.hpp>
#include <SushiProfiler/statistics/log2_histogram.hpp>
#include <SushiProfiler/statistics/sorted_samples.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns @p histogram as `2^e:count` words for its filled bins.
        std::string histogram_text(const Log2Histogram& histogram)
        {
            std::string text;
            if (histogram.nonpositive != 0U)
                text = "<=0:" + std::to_string(histogram.nonpositive);
            for (std::size_t bin = 0U; bin < histogram.counts.size(); ++bin)
            {
                if (histogram.counts[bin] == 0U)
                    continue;
                if (!text.empty())
                    text.push_back(' ');
                text += "2^" + std::to_string(histogram.first_exponent + static_cast<int>(bin)) +
                        ":" + std::to_string(histogram.counts[bin]);
            }
            return text;
        }

        /// @brief Appends the row of @p subject measured in @p unit over @p samples.
        void add_row(Table& table, std::string subject, std::string unit,
                     const std::vector<double>& samples)
        {
            if (samples.empty())
                return;
            const SortedSamples sorted(samples);
            Row row{Cell::text(std::move(subject)), Cell::text(std::move(unit)),
                    Cell::whole(static_cast<std::int64_t>(samples.size())),
                    Cell::real(sorted.percentile(50.0)), Cell::real(sorted.percentile(90.0)),
                    Cell::real(sorted.percentile(99.0)), Cell::real(sorted.maximum()),
                    Cell::real(jitter(samples)),
                    Cell::text(histogram_text(log2_histogram(samples)))};
            table.rows.push_back(std::move(row));
        }
    } // namespace

    std::string_view DistributionAnalysis::name() const noexcept
    {
        return "distribution";
    }

    Table DistributionAnalysis::run(const RunRecord& record, const AnalysisOptions& options) const
    {
        static_cast<void>(options);
        Table table;
        table.name = "distribution";
        table.add_column("subject");
        table.add_column("unit");
        table.add_column("count", Unit::Count);
        table.add_column("p50");
        table.add_column("p90");
        table.add_column("p99");
        table.add_column("max");
        table.add_column("jitter");
        table.add_column("histogram");
        for (const SubjectSamples& subject : subject_samples(record))
        {
            const std::string kind = subject.subject.substr(0U, subject.subject.find(':'));
            if (kind == "timeline" || kind == "zone" || kind == "counter" || kind == "value")
                add_row(table, subject.subject, subject.unit, subject.samples);
        }
        return table;
    }
} // namespace SushiProfiler
