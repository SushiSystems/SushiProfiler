/**************************************************************************/
/* period_breakdown.hpp                                                   */
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
 * @file period_breakdown.hpp
 * @brief Declares the breakdown of one period: self time per group, uncovered time, thread totals.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#pragma once

#include <SushiProfiler/record/period_record.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace SushiProfiler
{
    /// @brief The group a zone with no group of its own, under no grouped parent, falls in.
    inline constexpr const char* K_UNGROUPED_GROUP = "(ungrouped)";

    /// @brief The self time of one zone group in one period.
    struct GroupSelfTime
    {
        std::string group;            /**< The group's name. */
        std::uint64_t self_ns = 0U;   /**< Self time of every zone of the group. */
    };

    /// @brief What one thread's zones cost in one period, its lanes summed.
    struct ThreadTotal
    {
        std::uint32_t thread = 0U;           /**< The thread's index. */
        std::uint64_t root_inclusive_ns = 0U; /**< Inclusive time of its root zones. */
        std::uint64_t self_ns = 0U;          /**< Self time of all its zones. */
    };

    /// @brief One period's time split by zone group, with what no zone covered.
    struct PeriodBreakdown
    {
        /// @brief The marking thread's groups, largest self time first, then by name.
        std::vector<GroupSelfTime> groups;

        /// @brief The period's length less the marking thread's root time, never below 0.
        std::uint64_t uncovered_ns = 0U;

        /// @brief Every thread that ran zones, by ascending index.
        std::vector<ThreadTotal> threads;
    };

    /**
     * @brief Returns the group of node @p node of @p tree.
     *
     * The node's site group, else its nearest grouped ancestor's, else K_UNGROUPED_GROUP.
     */
    [[nodiscard]] SUSHIPROFILER_API std::string group_of_node(const std::vector<SiteRecord>& sites,
                                                             const ZoneTree& tree,
                                                             std::size_t node);

    /**
     * @brief Splits @p period's time by zone group, the arithmetic the `breakdown` analysis uses.
     * @param sites The site table that names each zone's group, indexed by SiteID.
     */
    [[nodiscard]] SUSHIPROFILER_API PeriodBreakdown break_down_period(
        const PeriodRecord& period, const std::vector<SiteRecord>& sites);
} // namespace SushiProfiler
