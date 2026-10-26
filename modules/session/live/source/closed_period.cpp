/**************************************************************************/
/* closed_period.cpp                                                      */
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
 * @file closed_period.cpp
 * @brief Copies a period for a live reader, keeping only the zone runs the cap allows.
 * @author Mustafa Garip
 * @see modules/session/live/README.md
 */

#include <SushiProfiler/live/closed_period.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns the indices of @p runs to keep under @p cap, in their original order.
        std::vector<std::size_t> kept_runs(const std::vector<ZoneRun>& runs, const RunCap& cap)
        {
            std::vector<std::size_t> kept;
            if (cap.max_runs == 0U)
                return kept;
            for (std::size_t index = 0U; index < runs.size(); ++index)
            {
                if (runs[index].end_ns - runs[index].begin_ns >= cap.min_run_ns)
                    kept.push_back(index);
            }
            if (kept.size() <= cap.max_runs)
                return kept;
            const auto longer = [&runs](std::size_t left, std::size_t right)
            {
                const std::uint64_t left_ns = runs[left].end_ns - runs[left].begin_ns;
                const std::uint64_t right_ns = runs[right].end_ns - runs[right].begin_ns;
                return left_ns != right_ns ? left_ns > right_ns : left < right;
            };
            std::nth_element(kept.begin(), kept.begin() + cap.max_runs, kept.end(), longer);
            kept.resize(cap.max_runs);
            std::sort(kept.begin(), kept.end());
            return kept;
        }
    } // namespace

    std::shared_ptr<const ClosedPeriod> make_closed_period(
        const PeriodRecord& period, std::uint64_t run, std::uint64_t sequence,
        std::uint64_t origin_steady_ns, const RunCap& cap, const LiveCounts& counts,
        std::shared_ptr<const NameSnapshot> names)
    {
        auto closed = std::make_shared<ClosedPeriod>();
        closed->run = run;
        closed->sequence = sequence;
        closed->origin_steady_ns = origin_steady_ns;
        closed->counts = counts;
        closed->names = std::move(names);
        // Copied by hand, runs excepted, so dropped runs are never copied; add new fields here.
        PeriodRecord& copy = closed->period;
        copy.index = period.index;
        copy.timeline = period.timeline;
        copy.thread = period.thread;
        copy.begin_ns = period.begin_ns;
        copy.end_ns = period.end_ns;
        copy.warmup = period.warmup;
        copy.budget_ns = period.budget_ns;
        copy.tags = period.tags;
        copy.facts = period.facts;
        copy.zones = period.zones;
        copy.spans = period.spans;
        copy.samples = period.samples;
        const std::vector<std::size_t> kept = kept_runs(period.runs, cap);
        copy.runs.reserve(kept.size());
        for (const std::size_t index : kept)
            copy.runs.push_back(period.runs[index]);
        closed->runs_omitted = period.runs.size() - kept.size();
        return closed;
    }
} // namespace SushiProfiler
