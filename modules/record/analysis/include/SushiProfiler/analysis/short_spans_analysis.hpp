/**************************************************************************/
/* short_spans_analysis.hpp                                               */
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
 * @file short_spans_analysis.hpp
 * @brief Declares the analysis of spans too short to pay for their launch.
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
     * @brief Reports the spans under the options' bound, and their cost at the launch floor when
     * known.
     */
    class SUSHIPROFILER_API ShortSpansAnalysis final : public Analysis
    {
        public:
            /// @brief Returns `short_spans`.
            [[nodiscard]] std::string_view name() const noexcept override;

            /// @brief Returns the table for @p record.
            [[nodiscard]] Table run(const RunRecord& record,
                                    const AnalysisOptions& options) const override;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
