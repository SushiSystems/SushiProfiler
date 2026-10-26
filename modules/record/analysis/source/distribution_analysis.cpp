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

#include <SushiProfiler/sites/site.hpp>
#include <SushiProfiler/statistics/dispersion.hpp>
#include <SushiProfiler/statistics/log2_histogram.hpp>
#include <SushiProfiler/statistics/sorted_samples.hpp>

#include "record_queries.hpp"

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

        /// @brief Returns the samples of site @p site in @p periods, period by period.
        std::vector<double> site_samples(const SiteRecord& site, SiteID id,
                                         const std::vector<const PeriodRecord*>& periods)
        {
            std::vector<double> samples;
            for (const PeriodRecord* const period : periods)
            {
                double value = 0.0;
                if (site.role == SiteRole::Counter)
                {
                    if (Detail::fold_counter(*period, id, site.op, value))
                        samples.push_back(value);
                }
                else if (site.role == SiteRole::Value)
                {
                    for (const SampleSeries& series : period->samples)
                    {
                        if (series.site == id)
                            samples.insert(samples.end(), series.values.begin(),
                                           series.values.end());
                    }
                }
                else if (Detail::runs_site(*period, id))
                {
                    samples.push_back(static_cast<double>(Detail::site_inclusive(*period, id)));
                }
            }
            return samples;
        }

        /// @brief Returns the subject prefix of @p role, or null for a role with no row.
        const char* prefix_of(SiteRole role) noexcept
        {
            switch (role)
            {
                case SiteRole::Zone: return "zone:";
                case SiteRole::LockWait: return "lock_wait:";
                case SiteRole::Counter: return "counter:";
                case SiteRole::Value: return "value:";
                case SiteRole::Instant: return nullptr;
                case SiteRole::Flow: return nullptr;
            }
            return nullptr;
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
        for (const NameID timeline : Detail::active_timelines(record))
        {
            add_row(table, "timeline:" + std::string(record.name_of(timeline)), "ns",
                    Detail::durations(Detail::active_periods(record, timeline)));
        }
        const std::vector<const PeriodRecord*> periods = Detail::active_periods(record);
        for (std::size_t id = 1U; id < record.sites.size(); ++id)
        {
            const SiteRecord& site = record.sites[id];
            const char* const prefix = prefix_of(site.role);
            if (prefix == nullptr)
                continue;
            const bool timed = site.role == SiteRole::Zone || site.role == SiteRole::LockWait;
            add_row(table, prefix + Detail::site_label(record, static_cast<SiteID>(id)),
                    timed ? std::string("ns") : site.unit,
                    site_samples(site, static_cast<SiteID>(id), periods));
        }
        return table;
    }
} // namespace SushiProfiler
