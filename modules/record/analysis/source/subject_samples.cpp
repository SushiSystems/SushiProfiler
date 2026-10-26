/**************************************************************************/
/* subject_samples.cpp                                                    */
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
 * @file subject_samples.cpp
 * @brief Gathers each subject's samples from the shared record queries.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/subject_samples.hpp>

#include <map>
#include <utility>

#include <SushiProfiler/sites/site.hpp>

#include "record_queries.hpp"

namespace SushiProfiler
{
    namespace
    {
        /// @brief Appends @p samples under @p subject when there are any.
        void add(std::vector<SubjectSamples>& out, std::string subject, std::string unit,
                 std::vector<double> samples)
        {
            if (!samples.empty())
            {
                out.push_back(
                    SubjectSamples{std::move(subject), std::move(unit), std::move(samples)});
            }
        }

        /// @brief Returns the periods of @p periods on @p timeline.
        std::vector<const PeriodRecord*> on_timeline(
            const std::vector<const PeriodRecord*>& periods, NameID timeline)
        {
            std::vector<const PeriodRecord*> kept;
            for (const PeriodRecord* const period : periods)
            {
                if (period->timeline == timeline)
                    kept.push_back(period);
            }
            return kept;
        }

        /// @brief Returns the samples of site @p id, whose role is not Flow, over @p periods.
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
                    continue;
                }
                if (site.role == SiteRole::Value)
                {
                    for (const SampleSeries& series : period->samples)
                    {
                        if (series.site == id)
                            samples.insert(samples.end(), series.values.begin(),
                                           series.values.end());
                    }
                    continue;
                }
                if (Detail::runs_site(*period, id))
                    samples.push_back(static_cast<double>(Detail::site_inclusive(*period, id)));
            }
            return samples;
        }
    } // namespace

    std::vector<SubjectSamples> subject_samples(const RunRecord& record,
                                                const std::vector<const PeriodRecord*>& periods)
    {
        std::vector<SubjectSamples> subjects;
        for (const NameID timeline : Detail::active_timelines(record))
        {
            const std::vector<const PeriodRecord*> kept = on_timeline(periods, timeline);
            const std::string name(record.name_of(timeline));
            add(subjects, "timeline:" + name, "ns", Detail::durations(kept));
            Detail::GroupSeries series = Detail::group_series(record, kept);
            for (auto& [group, times] : series.groups)
                add(subjects, "group:" + name + "/" + group, "ns", std::move(times));
            if (!kept.empty())
            {
                add(subjects, "group:" + name + "/" + Detail::K_UNCOVERED_LABEL, "ns",
                    std::move(series.uncovered));
            }
        }
        const char* const prefixes[] = {"zone:", "zone:", nullptr, "counter:", "value:", nullptr};
        for (std::size_t id = 1U; id < record.sites.size(); ++id)
        {
            const SiteRecord& site = record.sites[id];
            const char* const prefix = prefixes[static_cast<std::size_t>(site.role)];
            if (prefix == nullptr)
                continue;
            const bool timed = site.role == SiteRole::Zone || site.role == SiteRole::LockWait;
            add(subjects, prefix + Detail::site_label(record, static_cast<SiteID>(id)),
                timed ? std::string("ns") : site.unit,
                site_samples(site, static_cast<SiteID>(id), periods));
        }
        std::map<std::string, std::vector<double>> spans;
        for (const PeriodRecord* const period : periods)
        {
            for (std::size_t row = 0U; row < period->spans.size(); ++row)
            {
                const Span span = period->spans.span(row);
                spans[std::string(record.name_of(span.name))].push_back(
                    static_cast<double>(span.end_ns - span.start_ns));
            }
        }
        for (auto& [name, lengths] : spans)
            add(subjects, "span:" + name, "ns", std::move(lengths));
        for (const auto& [name, sites] : Detail::flow_sites(record))
            add(subjects, "flow:" + name, "ns", Detail::flow_latencies(sites, periods).totals);
        return subjects;
    }

    std::vector<SubjectSamples> subject_samples(const RunRecord& record)
    {
        return subject_samples(record, Detail::active_periods(record));
    }
} // namespace SushiProfiler
