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
 * @file verdict.hpp
 * @brief Declares the verdicts a record earns against checks.
 * @author Mustafa Garip
 * @see modules/record/checks/README.md
 */

#pragma once

#include <SushiProfiler/checks/check.hpp>
#include <SushiProfiler/record/run_record.hpp>

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
    [[nodiscard]] std::vector<Verdict> check(const RunRecord& record,
                                             const std::vector<Check>& checks);

    /// @brief Returns whether every verdict passed.
    [[nodiscard]] bool all_passed(const std::vector<Verdict>& verdicts) noexcept;
} // namespace SushiProfiler
