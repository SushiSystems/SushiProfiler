/**************************************************************************/
/* roofline_analysis.hpp                                                  */
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
 * @file roofline_analysis.hpp
 * @brief Declares the analysis of achieved FLOP/s and bytes/s against a device's ceilings.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#pragma once

#include <SushiProfiler/analysis/analysis.hpp>
#include <SushiProfiler/version/export.hpp>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Reports, per period, span name, track, node and dtype, the achieved FLOP/s and bytes/s
     * of spans that carry the integer attributes `flops` and `bytes`.
     *
     * With the environment facts `device.<track>.peak_flops` and `device.<track>.peak_bytes_per_s`
     * it also gives the bound min(peak_flops, intensity * peak_bytes_per_s) and the share achieved.
     */
    class SUSHIPROFILER_API RooflineAnalysis final : public Analysis
    {
        public:
            /// @brief Returns `roofline`.
            [[nodiscard]] std::string_view name() const noexcept override;

            /// @brief Returns the table for @p record.
            [[nodiscard]] Table run(const RunRecord& record,
                                    const AnalysisOptions& options) const override;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
