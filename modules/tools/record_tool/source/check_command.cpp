/**************************************************************************/
/* check_command.cpp                                                      */
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
 * @file check_command.cpp
 * @brief Reads a check file, takes each check's verdict on a record and prints them.
 * @author Mustafa Garip
 * @see modules/tools/record_tool/README.md
 */

#include "record_commands.hpp"

#include <cstddef>
#include <cstdio>
#include <optional>

#include <SushiProfiler/checks/verdict.hpp>
#include <SushiProfiler/record_tool/record_tool.hpp>

#include "tool_arguments.hpp"

namespace SushiProfiler::Detail
{
    namespace
    {
        /// @brief Returns @p value with six significant digits.
        std::string shown(double value)
        {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%.6g", value);
            return buffer;
        }
    } // namespace

    int check_command(const std::vector<std::string>& arguments, std::ostream& out,
                      std::ostream& err)
    {
        const ToolArguments parsed = parse_tool_arguments(arguments, {}, {});
        if (!parsed.error.empty() || parsed.positional.size() != 2U)
        {
            err << (parsed.error.empty() ? "check takes a record and a check file" : parsed.error)
                << '\n';
            return K_TOOL_USAGE;
        }
        int failure = K_TOOL_OK;
        const std::optional<RunRecord> record =
            read_record_file(parsed.positional[0], err, failure);
        if (!record)
            return failure;
        const std::string& checks_path = parsed.positional[1];
        const std::optional<std::string> text = read_text_file(checks_path);
        if (!text)
        {
            err << checks_path << ": could not be read\n";
            return K_TOOL_USAGE;
        }
        const ChecksReadResult checks = read_checks(*text);
        if (!checks.ok())
        {
            err << checks_path << ": " << checks.error << '\n';
            return K_TOOL_USAGE;
        }
        const std::vector<Verdict> verdicts = check(*record, checks.checks);
        std::size_t passed = 0U;
        for (const Verdict& verdict : verdicts)
        {
            const std::string statistic(statistic_word(verdict.check.statistic));
            if (verdict.passed)
            {
                ++passed;
                out << "pass  " << verdict.check.subject << ' ' << statistic << " = "
                    << shown(verdict.value) << '\n';
            }
            else
            {
                out << "FAIL  " << verdict.check.subject << ": " << verdict.reason << '\n';
            }
        }
        out << passed << " of " << verdicts.size() << " checks passed\n";
        return all_passed(verdicts) ? K_TOOL_OK : K_TOOL_FAILED;
    }
} // namespace SushiProfiler::Detail
