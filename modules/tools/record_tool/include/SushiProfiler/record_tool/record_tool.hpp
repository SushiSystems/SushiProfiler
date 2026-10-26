/**************************************************************************/
/* record_tool.hpp                                                        */
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
 * @file record_tool.hpp
 * @brief Declares the record tool: show, compare, check and validate run record files.
 * @author Mustafa Garip
 * @see modules/tools/record_tool/README.md
 */

#pragma once

#include <SushiProfiler/version/export.hpp>

#include <ostream>
#include <string>
#include <vector>

namespace SushiProfiler
{
    /// @brief The tool's exit code when the command did what was asked and found nothing wrong.
    inline constexpr int K_TOOL_OK = 0;

    /// @brief The tool's exit code when a record is invalid, a check failed or a compare refused.
    inline constexpr int K_TOOL_FAILED = 1;

    /// @brief The tool's exit code when the arguments are wrong or a file could not be read.
    inline constexpr int K_TOOL_USAGE = 2;

    /**
     * @brief Runs one command of the record tool on @p arguments, without the program name.
     *
     * The commands are `show`, `compare`, `check`, `validate` and `help`. Results go to @p out,
     * refusals and usage to @p err. Returns K_TOOL_OK, K_TOOL_FAILED or K_TOOL_USAGE.
     */
    [[nodiscard]] SUSHIPROFILER_API int run_record_tool(const std::vector<std::string>& arguments,
                                                        std::ostream& out, std::ostream& err);
} // namespace SushiProfiler
