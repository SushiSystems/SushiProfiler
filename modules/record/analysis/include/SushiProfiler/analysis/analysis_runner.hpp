/**************************************************************************/
/* analysis_runner.hpp                                                    */
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
 * @file analysis_runner.hpp
 * @brief Declares the runner that applies named analyses to a record.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#pragma once

#include <SushiProfiler/analysis/analysis_catalog.hpp>

#include <string>
#include <vector>

namespace SushiProfiler
{
    /// @brief The tables of one run, or why none were made.
    struct AnalysisRun
    {
        std::vector<Table> tables; /**< One table per name, in the order asked. */
        std::string error;         /**< Why the run was refused, or empty. */

        /// @brief Returns whether the run made its tables.
        [[nodiscard]] bool ok() const noexcept
        {
            return error.empty();
        }
    };

    /// @brief Runs analyses from one catalog.
    class AnalysisRunner final
    {
        public:
            /// @brief Builds a runner over @p catalog, which outlives it.
            explicit AnalysisRunner(const AnalysisCatalog& catalog) noexcept;

            /**
             * @brief Runs the analyses named in @p names over @p record, in order.
             *
             * Every name is checked before any analysis runs: an unknown one refuses the whole
             * run, naming it and listing the known names.
             */
            [[nodiscard]] AnalysisRun run(const RunRecord& record,
                                          const std::vector<std::string>& names,
                                          const AnalysisOptions& options) const;

        private:
            /// @brief The catalog the names are looked up in.
            const AnalysisCatalog& catalog_;
    };
} // namespace SushiProfiler
