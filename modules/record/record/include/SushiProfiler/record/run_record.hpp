/**************************************************************************/
/* run_record.hpp                                                         */
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
 * @file run_record.hpp
 * @brief Declares the run record: what one profiled run leaves, ready to analyse and compare.
 * @author Mustafa Garip
 * @see modules/record/record/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/record/period_record.hpp>
#include <SushiProfiler/sites/site.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace SushiProfiler
{
    /// @brief One recording site, with its strings owned.
    struct SiteRecord
    {
        std::string name;               /**< What the site measures. */
        std::string group;              /**< Its group, or empty to inherit its parent's. */
        std::string file;               /**< The source file. */
        std::string unit;               /**< A counter's or value's unit, or empty. */
        std::uint32_t line = 0U;        /**< The source line. */
        std::uint8_t level = 1U;        /**< Its density level. */
        SiteRole role = SiteRole::Zone; /**< What it records. */
        CounterOp op = CounterOp::Add;  /**< A counter's operation. */

        /// @brief Returns whether every field is equal.
        [[nodiscard]] bool operator==(const SiteRecord& other) const;
    };

    /// @brief One thread that recorded, and what it lost.
    struct ThreadRecord
    {
        std::uint32_t index = 0U;  /**< The thread's index. */
        std::string name;          /**< Its name, or empty. */
        std::uint64_t dropped = 0U; /**< Events its full ring dropped. */
        std::uint64_t late = 0U;   /**< Events that began in a period already closed. */
        std::uint64_t unmatched = 0U; /**< Zones that did not nest, lane ends with none open. */

        /// @brief Returns whether every field is equal.
        [[nodiscard]] bool operator==(const ThreadRecord& other) const;
    };

    /// @brief The clock the record's times were read on.
    struct ClockRecord
    {
        std::string source;               /**< `tsc` or `steady`. */
        double nanoseconds_per_tick = 1.0; /**< The calibration's slope. */
        double read_cost_ns = 0.0;        /**< The measured cost of one read. */

        /// @brief Returns whether every field is equal.
        [[nodiscard]] bool operator==(const ClockRecord& other) const;
    };

    /// @brief What the run lost outside any one thread.
    struct RecordDrops
    {
        std::uint64_t outside_periods = 0U; /**< Events that began outside every period. */
        std::uint64_t open_at_stop = 0U;    /**< Zones still open when the session stopped. */
        std::uint64_t spans_dropped = 0U;   /**< Spans a source could not place. */

        /// @brief Returns whether every field is equal.
        [[nodiscard]] bool operator==(const RecordDrops& other) const;
    };

    /**
     * @brief Holds one run: its environment, its tables and its periods.
     *
     * Times are nanoseconds since the session's first calibration point. `names[i]` is the text
     * of NameID `i` and `sites[i]` the site of SiteID `i`; index 0 of each is empty.
     */
    struct RunRecord
    {
        std::vector<Fact> environment;     /**< The host, the libraries and the configuration. */
        std::vector<Fact> build;           /**< Build type, optimisation and sanitisers. */
        std::vector<Fact> tags;            /**< The run's own tags. */
        ClockRecord clock;                 /**< The clock. */
        std::vector<std::string> names;    /**< Interned names by NameID. */
        std::vector<SiteRecord> sites;     /**< Sites by SiteID. */
        std::vector<std::string> tracks;   /**< Track names by TrackID. */
        std::vector<ThreadRecord> threads; /**< Every thread that recorded. */
        std::vector<PeriodRecord> periods; /**< The recorded periods, in order. */
        RecordDrops drops;                 /**< What was lost outside any thread. */

        /// @brief Returns the text of @p name, or an empty view for an unknown id.
        [[nodiscard]] std::string_view name_of(NameID name) const noexcept;

        /// @brief Returns site @p site, or null for an unknown id.
        [[nodiscard]] const SiteRecord* site(SiteID site) const noexcept;

        /// @brief Returns the NameID of @p text, or 0 when the record holds no such name.
        [[nodiscard]] NameID find_name(std::string_view text) const noexcept;

        /// @brief Returns whether every field is equal.
        [[nodiscard]] bool operator==(const RunRecord& other) const;
    };
} // namespace SushiProfiler
