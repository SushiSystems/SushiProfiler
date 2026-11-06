/**************************************************************************/
/* verdict.hpp                                                            */
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
 * @file verdict.hpp
 * @brief Declares the verdicts a record earns against checks.
 * @author Mustafa Garip
 * @see modules/record/checks/README.md
 */

#pragma once

#include <SushiProfiler/checks/check.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/version/export.hpp>

#include <string>
#include <vector>

namespace SushiProfiler
{
    /// @brief What one check found.
    struct Verdict
    {
        Check check;        /**< The check. */
        bool passed = false; /**< Whether the value is within the bounds. */
        double value = 0.0; /**< The statistic's value, or 0 without samples. */
        std::string reason; /**< Why it failed, or empty. */
    };

    /**
     * @brief Returns one verdict per check of @p checks over @p record's active periods.
     *
     * A check whose subject has no samples in the record fails.
     */
    [[nodiscard]] SUSHIPROFILER_API std::vector<Verdict> check(const RunRecord& record,
                                                               const std::vector<Check>& checks);

    /// @brief Returns whether every verdict passed.
    [[nodiscard]] SUSHIPROFILER_API bool all_passed(const std::vector<Verdict>& verdicts) noexcept;
} // namespace SushiProfiler
