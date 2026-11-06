/**************************************************************************/
/* compare_records.hpp                                                    */
/**************************************************************************/
/*                          This file is part of:                         */
/*                              SushiProfiler                             */
/*              https://github.com/sushisystems/sushiprofiler             */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file compare_records.hpp
 * @brief Declares the row-by-row comparison of two records with bootstrap intervals.
 * @author Mustafa Garip
 * @see modules/record/compare/README.md
 */

#pragma once

#include <SushiProfiler/analysis/table.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/statistics/bootstrap.hpp>
#include <SushiProfiler/version/export.hpp>

#include <string>

namespace SushiProfiler
{
    /// @brief How two records are compared.
    struct CompareOptions
    {
        BootstrapSettings bootstrap;       /**< How each median's interval is drawn. */
        bool allow_build_mismatch = false; /**< Whether differing build facts are accepted. */
    };

    /// @brief The comparison table, or why the records were not compared.
    struct Comparison
    {
        Table table;       /**< One row per subject both records hold. */
        std::string error; /**< Why the comparison was refused, or empty. */

        /// @brief Returns whether the records were compared.
        [[nodiscard]] bool ok() const noexcept
        {
            return error.empty();
        }
    };

    /**
     * @brief Compares @p candidate against @p baseline, subject by subject.
     *
     * A row holds both medians with their bootstrap intervals and the ratio of medians; a change
     * is named only when the intervals do not overlap. Tagged periods are compared by tag set,
     * and differing build facts refuse the comparison unless the options allow them.
     */
    [[nodiscard]] SUSHIPROFILER_API Comparison compare_records(
        const RunRecord& baseline, const RunRecord& candidate, const CompareOptions& options = {});
} // namespace SushiProfiler
