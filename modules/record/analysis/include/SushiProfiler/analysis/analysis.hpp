/**************************************************************************/
/* analysis.hpp                                                           */
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
 * @file analysis.hpp
 * @brief Declares the interface every analysis implements, and the options they share.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#pragma once

#include <SushiProfiler/analysis/table.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/statistics/bootstrap.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace SushiProfiler
{
    /// @brief The settings every analysis reads; each uses the ones its question needs.
    struct AnalysisOptions
    {
        BootstrapSettings bootstrap;          /**< How intervals are drawn. */
        std::size_t top = 20U;                /**< How many rows a ranking keeps. */
        std::vector<std::string> group_by;    /**< Attribute keys `top_spans` groups by. */
        std::uint64_t short_span_ns = 10'000U; /**< The bound `short_spans` counts under. */
        std::uint64_t launch_floor_ns = 0U;   /**< The device's launch cost, 0 when unknown. */
        const RunRecord* baseline = nullptr;  /**< A run without profiling, for `overhead`. */
    };

    /// @brief Answers one question about a run record as a table.
    class Analysis
    {
        public:
            virtual ~Analysis() = default;

            /// @brief Returns the name the catalog knows it by.
            [[nodiscard]] virtual std::string_view name() const noexcept = 0;

            /// @brief Returns the table for @p record.
            /// @pre @p record was accepted by `read_record` or built to the same rules.
            [[nodiscard]] virtual Table run(const RunRecord& record,
                                            const AnalysisOptions& options) const = 0;

        protected:
            Analysis() = default;
            Analysis(const Analysis&) = default;
            Analysis& operator=(const Analysis&) = default;
            Analysis(Analysis&&) = default;
            Analysis& operator=(Analysis&&) = default;
    };
} // namespace SushiProfiler
