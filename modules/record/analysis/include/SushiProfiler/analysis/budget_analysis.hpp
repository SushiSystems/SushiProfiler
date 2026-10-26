/**************************************************************************/
/* budget_analysis.hpp                                                    */
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
 * @file budget_analysis.hpp
 * @brief Declares the analysis of how much of its budget each period used.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#pragma once

#include <SushiProfiler/analysis/analysis.hpp>

namespace SushiProfiler
{
    /**
     * @brief Reports, per budgeted timeline, the used fraction, the periods over budget and the
     * worst period.
     */
    class BudgetAnalysis final : public Analysis
    {
        public:
            /// @brief Returns `budget`.
            [[nodiscard]] std::string_view name() const noexcept override;

            /// @brief Returns the table for @p record.
            [[nodiscard]] Table run(const RunRecord& record,
                                    const AnalysisOptions& options) const override;
    };
} // namespace SushiProfiler
