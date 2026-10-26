/**************************************************************************/
/* profiler_config.hpp                                                    */
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
 * @file profiler_config.hpp
 * @brief Declares the profiler configuration and its strict JSON reader.
 * @author Mustafa Garip
 * @see docs/reference/CONFIG_FORMAT.md
 */

#pragma once

#include <SushiProfiler/json/json_value.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SushiProfiler
{
    /// @brief The version of the configuration format this build reads.
    inline constexpr std::uint32_t K_CONFIG_FORMAT_VERSION = 1U;

    /// @brief Which periods of the followed timeline are recorded, as PyTorch's schedule.
    struct WindowSchedule
    {
        std::uint32_t skip = 0U;   /**< Periods ignored before the first cycle. */
        std::uint32_t wait = 0U;   /**< Periods not recorded at the start of each cycle. */
        std::uint32_t warmup = 3U; /**< Periods recorded and marked warm-up. */
        std::uint32_t active = 20U; /**< Periods recorded; at least 1. */
        std::uint32_t repeat = 1U; /**< Cycles; 0 repeats until the session stops. */
    };

    /// @brief A part the configuration names, such as a source or a writer, and its settings.
    struct NamedSettings
    {
        std::string name;   /**< The name the registry knows it by. */
        JsonValue settings; /**< Its settings, an object the part reads itself. */
    };

    /// @brief How much the capture path may hold.
    struct BufferSettings
    {
        std::uint32_t ring_events = 65'536U;      /**< Events per thread ring, 64 to 2^24. */
        std::uint64_t attribute_bytes = 1'048'576U; /**< A limit no part applies yet (RW-6). */
        std::uint32_t linger_ms = 100U;           /**< How long a period stays open, to 60 000. */
    };

    /// @brief Everything a session is started with; every field has a default.
    struct ProfilerConfig
    {
        bool enabled = true;                  /**< Whether the session records at all. */
        std::uint8_t level = 2U;              /**< The runtime density level, 0 to 3. */
        std::vector<std::string> groups;      /**< Site groups recorded; empty records all. */
        std::string timeline;                 /**< The timeline the window follows, or empty. */
        WindowSchedule window;                /**< Which of its periods are recorded. */
        std::vector<NamedSettings> sources;   /**< The sources, in file order. */
        std::vector<std::string> analyses;    /**< The analyses run on the record. */
        std::vector<NamedSettings> writers;   /**< The writers, in file order. */
        std::string output;                   /**< The folder outputs go in, or empty. */
        BufferSettings buffers;               /**< The capture path's sizes. */
    };

    /// @brief A configuration read from text, or why it was refused.
    struct ConfigReadResult
    {
        std::optional<ProfilerConfig> config; /**< The configuration, when the text holds one. */
        std::string error;                    /**< The refusal with its JSON path. */

        /// @brief Returns whether the configuration was read.
        [[nodiscard]] bool ok() const noexcept
        {
            return config.has_value();
        }
    };

    /**
     * @brief Reads a configuration; a key left out keeps its default.
     *
     * Refused, each with its path: a version other than K_CONFIG_FORMAT_VERSION or none, an
     * unknown key, a value of the wrong kind, and a value out of the range its field states.
     */
    [[nodiscard]] SUSHIPROFILER_API ConfigReadResult read_profiler_config(std::string_view text);
} // namespace SushiProfiler
