/**************************************************************************/
/* period_record.hpp                                                      */
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
 * @file period_record.hpp
 * @brief Declares one recorded period: its times, tags, zone trees, spans and samples.
 * @author Mustafa Garip
 * @see modules/record/record/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/record/sample_series.hpp>
#include <SushiProfiler/record/span_table.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstdint>
#include <string>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief A named fact, such as an environment entry or a period's execution layer.
    struct SUSHIPROFILER_API Fact
    {
        std::string name;  /**< What the fact is about. */
        std::string value; /**< Its value, as text. */

        /// @brief Returns whether name and value are equal.
        [[nodiscard]] bool operator==(const Fact& other) const;
    };

    /// @brief One tag of a period, as interned key and value.
    struct SUSHIPROFILER_API Tag
    {
        NameID key = 0U;   /**< The tag's key. */
        NameID value = 0U; /**< The tag's value. */

        /// @brief Returns whether key and value are equal.
        [[nodiscard]] bool operator==(const Tag& other) const;
    };

    /// @brief One node of a zone tree: every call of one site, under one parent, merged.
    struct SUSHIPROFILER_API ZoneNode
    {
        SiteID site = 0U;              /**< The zone's site. */
        NameID name = 0U;              /**< The zone's dynamic name, or 0 for the site's. */
        std::int32_t parent = -1;      /**< The parent node's index, or -1 at the root. */
        std::uint64_t inclusive_ns = 0U; /**< Time inside the zone, children included. */
        std::uint64_t self_ns = 0U;    /**< Time inside the zone outside its children. */
        std::uint64_t calls = 0U;      /**< How many times it ran under this parent. */

        /// @brief Returns whether every field is equal.
        [[nodiscard]] bool operator==(const ZoneNode& other) const;
    };

    /// @brief The zones one thread ran in one lane of a period, as a tree; parents come first.
    struct SUSHIPROFILER_API ZoneTree
    {
        std::uint32_t thread = 0U;    /**< The thread's index. */
        NameID lane = 0U;             /**< The lane, or 0 outside any lane. */
        std::vector<ZoneNode> nodes;  /**< The nodes, each after its parent. */

        /// @brief Returns whether thread, lane and nodes are equal.
        [[nodiscard]] bool operator==(const ZoneTree& other) const;
    };

    /// @brief One run of one zone on one thread, its times on the record's clock.
    struct SUSHIPROFILER_API ZoneRun
    {
        std::uint32_t thread = 0U;   /**< The thread's index. */
        NameID lane = 0U;            /**< The lane, or 0. */
        SiteID site = 0U;            /**< The zone's site. */
        NameID name = 0U;            /**< Its dynamic name, or 0. */
        std::uint64_t begin_ns = 0U; /**< When it began. */
        std::uint64_t end_ns = 0U;   /**< When it ended. */
        std::uint32_t depth = 0U;    /**< How many zones of the lane were open around it. */

        /// @brief Returns whether every field is equal.
        [[nodiscard]] bool operator==(const ZoneRun& other) const;
    };

    /// @brief One period of a timeline, as recorded.
    struct SUSHIPROFILER_API PeriodRecord
    {
        std::uint64_t index = 0U;      /**< The period's index on its timeline. */
        NameID timeline = 0U;          /**< The timeline. */
        std::uint32_t thread = 0U;     /**< The thread that marked it. */
        std::uint64_t begin_ns = 0U;   /**< Its start. */
        std::uint64_t end_ns = 0U;     /**< Its end, the next period's start. */
        bool warmup = false;           /**< Whether the window marked it as warm-up. */
        std::uint64_t budget_ns = 0U;  /**< Its budget, or 0 for none. */
        std::vector<Tag> tags;         /**< The tags attached to it. */
        std::vector<Fact> facts;       /**< Facts its sources reported for it. */
        std::vector<ZoneTree> zones;   /**< One tree per thread and lane that ran zones. */
        std::vector<ZoneRun> runs;     /**< Every zone run of an active period; none if warm-up. */
        SpanTable spans;               /**< The spans its sources reported. */
        std::vector<SampleSeries> samples; /**< One series per site and thread. */

        /// @brief Returns the period's length.
        [[nodiscard]] std::uint64_t duration_ns() const noexcept
        {
            return end_ns - begin_ns;
        }

        /// @brief Returns whether every field is equal.
        [[nodiscard]] bool operator==(const PeriodRecord& other) const;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
