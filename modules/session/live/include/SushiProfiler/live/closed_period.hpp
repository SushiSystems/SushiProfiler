/**************************************************************************/
/* closed_period.hpp                                                      */
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
 * @file closed_period.hpp
 * @brief Declares the immutable period a live reader receives, and how it is made.
 * @author Mustafa Garip
 * @see modules/session/live/README.md
 */

#pragma once

#include <SushiProfiler/live/name_snapshot.hpp>
#include <SushiProfiler/record/period_record.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstdint>
#include <memory>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief What the run had lost as of one period close; every field only grows.
    struct LiveCounts
    {
        std::uint64_t late = 0U;          /**< Events that began in a period already closed. */
        std::uint64_t dropped = 0U;       /**< Events the rings dropped. */
        std::uint64_t spans_dropped = 0U; /**< Spans a source could not place. */
        std::uint64_t outside = 0U;       /**< Events that began outside every period. */
    };

    /// @brief How many zone runs a published period keeps.
    struct RunCap
    {
        std::uint32_t max_runs = 0U;   /**< The most runs kept; the longest win. */
        std::uint64_t min_run_ns = 0U; /**< Runs shorter than this are dropped first. */
    };

    /**
     * @brief One closed period, immutable, with the names its ids refer to.
     *
     * A reader holds the shared pointer for as long as it likes; the session never changes or
     * frees what it points at while a reader holds it.
     */
    struct ClosedPeriod
    {
        std::uint64_t run = 0U;              /**< The session run it belongs to, from 1. */
        std::uint64_t sequence = 0U;         /**< 0 for the first period published in its run. */
        std::uint64_t origin_steady_ns = 0U; /**< Add to a record time for the steady clock. */
        PeriodRecord period;                 /**< The period, its zone runs capped. */
        std::uint64_t runs_omitted = 0U;     /**< Zone runs the cap left out. */
        LiveCounts counts;                   /**< The run's losses as of this close. */
        std::shared_ptr<const NameSnapshot> names; /**< What its ids mean. */
    };

    /**
     * @brief Copies @p period into a ClosedPeriod, keeping the zone runs @p cap allows.
     *
     * Runs shorter than `cap.min_run_ns` go first; if more than `cap.max_runs` remain, the
     * longest are kept. Kept runs keep their order. Every other field is copied as it is.
     */
    [[nodiscard]] SUSHIPROFILER_API std::shared_ptr<const ClosedPeriod> make_closed_period(
        const PeriodRecord& period, std::uint64_t run, std::uint64_t sequence,
        std::uint64_t origin_steady_ns, const RunCap& cap, const LiveCounts& counts,
        std::shared_ptr<const NameSnapshot> names);
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
