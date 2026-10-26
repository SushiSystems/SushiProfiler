/**************************************************************************/
/* compare_command.cpp                                                    */
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
 * @file compare_command.cpp
 * @brief Compares a candidate record with a baseline and prints every row.
 * @author Mustafa Garip
 * @see modules/tools/record_tool/README.md
 */

#include "record_commands.hpp"

#include <limits>
#include <optional>

#include <SushiProfiler/compare/compare_records.hpp>
#include <SushiProfiler/record_tool/record_tool.hpp>
#include <SushiProfiler/record_writers/terminal_writer.hpp>

#include "tool_arguments.hpp"

namespace SushiProfiler::Detail
{
    int compare_command(const std::vector<std::string>& arguments, std::ostream& out,
                        std::ostream& err)
    {
        const ToolArguments parsed = parse_tool_arguments(arguments, {}, {"allow-build-mismatch"});
        if (!parsed.error.empty() || parsed.positional.size() != 2U)
        {
            err << (parsed.error.empty() ? "compare takes a baseline and a candidate record"
                                         : parsed.error)
                << '\n';
            return K_TOOL_USAGE;
        }
        int failure = K_TOOL_OK;
        const std::optional<RunRecord> baseline =
            read_record_file(parsed.positional[0], err, failure);
        if (!baseline)
            return failure;
        const std::optional<RunRecord> candidate =
            read_record_file(parsed.positional[1], err, failure);
        if (!candidate)
            return failure;
        CompareOptions options;
        options.allow_build_mismatch = parsed.flags.count("allow-build-mismatch") != 0U;
        const Comparison comparison = compare_records(*baseline, *candidate, options);
        if (!comparison.ok())
        {
            err << comparison.error << '\n';
            return K_TOOL_FAILED;
        }
        TerminalWriter writer(out, std::numeric_limits<std::size_t>::max());
        return writer.write(*candidate, {comparison.table}) == WriteStatus::Written
                   ? K_TOOL_OK
                   : K_TOOL_FAILED;
    }
} // namespace SushiProfiler::Detail
