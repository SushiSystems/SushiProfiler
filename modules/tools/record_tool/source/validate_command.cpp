/**************************************************************************/
/* validate_command.cpp                                                   */
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
 * @file validate_command.cpp
 * @brief Reads a record strictly and says what it holds or why it is refused.
 * @author Mustafa Garip
 * @see modules/tools/record_tool/README.md
 */

#include "record_commands.hpp"

#include <optional>

#include <SushiProfiler/record_tool/record_tool.hpp>

#include "tool_arguments.hpp"

namespace SushiProfiler::Detail
{
    int validate_command(const std::vector<std::string>& arguments, std::ostream& out,
                         std::ostream& err)
    {
        const ToolArguments parsed = parse_tool_arguments(arguments, {}, {});
        if (!parsed.error.empty() || parsed.positional.size() != 1U)
        {
            err << (parsed.error.empty() ? "validate takes one record" : parsed.error) << '\n';
            return K_TOOL_USAGE;
        }
        int failure = K_TOOL_OK;
        const std::optional<RunRecord> record =
            read_record_file(parsed.positional[0], err, failure);
        if (!record)
            return failure;
        out << parsed.positional[0] << ": valid, " << record->periods.size() << " periods, "
            << record->threads.size() << " threads\n";
        return K_TOOL_OK;
    }
} // namespace SushiProfiler::Detail
