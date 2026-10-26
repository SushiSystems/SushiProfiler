/**************************************************************************/
/* record_queries.hpp                                                     */
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
 * @file record_queries.hpp
 * @brief Declares the lookups several analyses share: active periods, labels, groups and folds.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#pragma once

#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/sites/site.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace SushiProfiler::Detail
{
    /// @brief The group a zone with no group of its own, under no grouped parent, falls in.
    inline constexpr const char* K_UNGROUPED = "(ungrouped)";

    /// @brief The label of the time no root zone covered.
    inline constexpr const char* K_UNCOVERED_LABEL = "(uncovered)";

    /// @brief Returns the timelines that have an active period, ordered by name.
    [[nodiscard]] std::vector<NameID> active_timelines(const RunRecord& record);

    /// @brief Returns the periods of @p timeline that are not warm-up, in record order.
    [[nodiscard]] std::vector<const PeriodRecord*> active_periods(const RunRecord& record,
                                                                  NameID timeline);

    /// @brief Returns every period that is not warm-up, in record order.
    [[nodiscard]] std::vector<const PeriodRecord*> active_periods(const RunRecord& record);

    /// @brief Returns the site's name, or `site <id>` when it has none.
    [[nodiscard]] std::string site_label(const RunRecord& record, SiteID site);

    /// @brief Returns the label of node @p node: its dynamic name, else its site's.
    [[nodiscard]] std::string node_label(const RunRecord& record, const ZoneNode& node);

    /// @brief Returns the group of node @p node, inherited from the nearest grouped ancestor.
    [[nodiscard]] std::string group_of(const RunRecord& record, const ZoneTree& tree,
                                       std::size_t node);

    /// @brief Returns whether any node of @p period runs site @p site.
    [[nodiscard]] bool runs_site(const PeriodRecord& period, SiteID site) noexcept;

    /// @brief Returns the inclusive time of @p site in @p period, nested calls counted once.
    [[nodiscard]] std::uint64_t site_inclusive(const PeriodRecord& period, SiteID site) noexcept;

    /// @brief Folds the samples of counter @p site in @p period by @p op into @p out.
    /// @return false when the period holds no sample of the site.
    [[nodiscard]] bool fold_counter(const PeriodRecord& period, SiteID site, CounterOp op,
                                    double& out) noexcept;

    /// @brief One timeline's self time per zone group per period, and the time no zone covered.
    struct GroupSeries
    {
        std::map<std::string, std::vector<double>> groups; /**< Self time per period, by group. */
        std::vector<double> uncovered;                     /**< Uncovered time per period. */
    };

    /**
     * @brief Returns the group series of @p periods, summed over each marking thread's trees.
     *
     * Every series has one entry per period, 0 where a group did not run.
     */
    [[nodiscard]] GroupSeries group_series(const RunRecord& record,
                                           const std::vector<const PeriodRecord*>& periods);

    /// @brief One flow's latencies: from each item's begin to its end, and between its points.
    struct FlowLatencies
    {
        std::vector<double> totals; /**< Begin to end, one per finished item. */
        std::vector<double> steps;  /**< Between successive points of an item. */
    };

    /// @brief Returns the latencies of flow site @p site over @p periods, points in time order.
    [[nodiscard]] FlowLatencies flow_latencies(SiteID site,
                                               const std::vector<const PeriodRecord*>& periods);

    /// @brief Returns the durations of @p periods, as reals.
    [[nodiscard]] std::vector<double> durations(const std::vector<const PeriodRecord*>& periods);
} // namespace SushiProfiler::Detail
