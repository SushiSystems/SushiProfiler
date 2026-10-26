/**************************************************************************/
/* site.hpp                                                               */
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
 * @file site.hpp
 * @brief Declares a site, the static description of one place in the source that records.
 * @author Mustafa Garip
 * @see modules/capture/sites/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>

#include <atomic>
#include <cstdint>

namespace SushiProfiler
{
    /// @brief Names how a counter's samples fold into one number per period.
    enum class CounterOp : std::uint8_t
    {
        Add, /**< The sum of the samples. */
        Set, /**< The last sample. */
        Max, /**< The largest sample. */
        Min  /**< The smallest sample. */
    };

    /// @brief Names what a site records.
    enum class SiteRole : std::uint8_t
    {
        Zone,     /**< A time range. */
        LockWait, /**< The time a contended lock waited. */
        Instant,  /**< An occurrence. */
        Counter,  /**< A number folded by its operation. */
        Value,    /**< A sample kept as it is. */
        Flow      /**< A point on an item's path across threads. */
    };

    /**
     * @brief Describes one place in the source that records.
     *
     * Built as a `static constexpr` object by the recording macros. `group` and `unit` may be
     * null; every string outlives the program, and the registry copies them on registration.
     */
    struct Site
    {
        const char* name;    /**< What the site measures. */
        const char* group;   /**< The group its time is charged to, or null to inherit. */
        const char* file;    /**< The source file. */
        std::uint32_t line;  /**< The source line. */
        std::uint8_t level;  /**< The density level: 1 stages, 2 fine, 3 per-element detail. */
        SiteRole role;       /**< What it records. */
        CounterOp op;        /**< A counter's operation; Add for every other role. */
        const char* unit;    /**< A counter's or value's unit, or null. */
    };

    /// @brief One call site's site and the id the registry gave it, cached after first use.
    struct SiteHandle
    {
        const Site* site;              /**< The site this handle registers. */
        std::atomic<SiteID> id{0U};    /**< 0 until registered; see SiteRegistry. */
    };
} // namespace SushiProfiler
