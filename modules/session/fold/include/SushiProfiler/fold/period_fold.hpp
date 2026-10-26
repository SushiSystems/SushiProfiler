/**************************************************************************/
/* period_fold.hpp                                                        */
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
 * @file period_fold.hpp
 * @brief Declares the fold of one period's events into zone trees, samples and zone runs.
 * @author Mustafa Garip
 * @see modules/session/fold/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/record/period_record.hpp>
#include <SushiProfiler/record/sample_series.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstdint>
#include <vector>

namespace SushiProfiler
{
    /// @brief One event of a period, its times on the record's clock, with the lane it ran in.
    struct FoldEvent
    {
        TraceEvent event; /**< The event, as RecordClock::to_record() returns it. */
        NameID lane = 0U; /**< Its lane, as LaneTracker::lane_of() returns it. */
    };

    /// @brief One thread's events of a period, in ring order.
    struct ThreadEvents
    {
        std::uint32_t thread = 0U;     /**< The thread's index. */
        std::vector<FoldEvent> events; /**< Its events that begin in the period. */
    };

    /// @brief How many of one thread's events a fold could not place.
    struct ThreadCount
    {
        std::uint32_t thread = 0U; /**< The thread's index. */
        std::uint64_t count = 0U;  /**< The events. */
    };

    /// @brief What one period's events fold into.
    struct PeriodFold
    {
        std::vector<ZoneTree> zones;         /**< One tree per thread and lane, in that order. */
        std::vector<SampleSeries> samples;   /**< One series per site and thread, in that order. */
        std::vector<ZoneRun> instances; /**< Every zone run, by thread, lane and begin. */
        std::vector<ThreadCount> unmatched;  /**< Zones that did not nest, per thread with any. */
    };

    /**
     * @brief Folds @p threads into zone trees, sample series and zone runs.
     *
     * Zones nest by time within a thread and lane; calls of one site and name under one parent
     * merge into one node, and self time is the zone's time outside its children. A zone that
     * ends after the zone open around it, or before it begins, is unmatched: the first becomes
     * that zone's sibling, the second is dropped. Period and lane events are not folded.
     */
    [[nodiscard]] SUSHIPROFILER_API PeriodFold fold_period(
        const std::vector<ThreadEvents>& threads);
} // namespace SushiProfiler
