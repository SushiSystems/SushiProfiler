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
 * @file compare_records.hpp
 * @brief Declares the row-by-row comparison of two records with bootstrap intervals.
 * @author Mustafa Garip
 * @see modules/record/compare/README.md
 */

#pragma once

#include <SushiProfiler/analysis/table.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/statistics/bootstrap.hpp>

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
    [[nodiscard]] Comparison compare_records(const RunRecord& baseline, const RunRecord& candidate,
                                             const CompareOptions& options = {});
} // namespace SushiProfiler
