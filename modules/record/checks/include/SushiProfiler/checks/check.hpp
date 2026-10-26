/**************************************************************************/
/* check.hpp                                                              */
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
 * @file check.hpp
 * @brief Declares a check, the statistics it bounds, and how checks are read from JSON.
 * @author Mustafa Garip
 * @see modules/record/checks/README.md
 */

#pragma once

#include <SushiProfiler/version/export.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SushiProfiler
{
    /// @brief The statistic of a subject's samples a check bounds.
    enum class Statistic : std::uint8_t
    {
        Median, /**< The median. */
        P50,    /**< The 50th percentile, the median by another name. */
        P90,    /**< The 90th percentile. */
        P95,    /**< The 95th percentile. */
        P99,    /**< The 99th percentile. */
        Min,    /**< The smallest sample. */
        Max     /**< The largest sample. */
    };

    /// @brief One bound on one statistic of one subject.
    struct Check
    {
        std::string subject;                 /**< A subject name, such as `timeline:step`. */
        Statistic statistic = Statistic::Median; /**< The statistic bounded. */
        std::optional<double> at_most;       /**< The largest value that passes. */
        std::optional<double> at_least;      /**< The smallest value that passes. */
    };

    /// @brief Checks read from text, or why they were refused.
    struct ChecksReadResult
    {
        std::vector<Check> checks; /**< The checks, in file order. */
        std::string error;         /**< The refusal with its JSON path, or empty. */

        /// @brief Returns whether the checks were read.
        [[nodiscard]] bool ok() const noexcept
        {
            return error.empty();
        }
    };

    /// @brief Returns the word for @p statistic: median, p50, p90, p95, p99, min or max.
    [[nodiscard]] SUSHIPROFILER_API std::string_view statistic_word(Statistic statistic) noexcept;

    /**
     * @brief Reads a JSON array of checks.
     *
     * Each holds `subject`, `statistic` and at least one bound. A time subject (`timeline:`,
     * `group:`, `zone:`, `span:`, `flow:`) takes `at_most_ns` and `at_least_ns`; `counter:` and
     * `value:` take `at_most` and `at_least`. Anything else is refused, naming its path.
     */
    [[nodiscard]] SUSHIPROFILER_API ChecksReadResult read_checks(std::string_view text);
} // namespace SushiProfiler
