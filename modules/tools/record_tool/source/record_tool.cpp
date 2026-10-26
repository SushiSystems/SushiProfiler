/**************************************************************************/
/* record_tool.cpp                                                        */
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
 * @file record_tool.cpp
 * @brief Finds the command the first argument names and runs it on the rest.
 * @author Mustafa Garip
 * @see modules/tools/record_tool/README.md
 */

#include <SushiProfiler/record_tool/record_tool.hpp>

#include <array>
#include <string_view>

#include "record_commands.hpp"

namespace SushiProfiler
{
    namespace
    {
        /// @brief Runs one command on its arguments.
        using Command = int (*)(const std::vector<std::string>&, std::ostream&, std::ostream&);

        /// @brief A command and the word that names it.
        struct NamedCommand
        {
            std::string_view name; /**< The word. */
            Command run;           /**< The command. */
        };

        /// @brief Every command, in the order usage lists them.
        constexpr std::array<NamedCommand, 4> K_COMMANDS = {
            NamedCommand{"show", &Detail::show_command},
            NamedCommand{"compare", &Detail::compare_command},
            NamedCommand{"check", &Detail::check_command},
            NamedCommand{"validate", &Detail::validate_command}};

        /// @brief The usage text.
        constexpr std::string_view K_USAGE =
            "usage: sp_record <command> ...\n"
            "  show <record> [--analysis NAME]... [--top N] [--group-by KEY]... "
            "[--baseline RECORD]\n"
            "  compare <baseline> <candidate> [--allow-build-mismatch]\n"
            "  check <record> <checks>\n"
            "  validate <record>\n";
    } // namespace

    int run_record_tool(const std::vector<std::string>& arguments, std::ostream& out,
                        std::ostream& err)
    {
        if (arguments.empty())
        {
            err << K_USAGE;
            return K_TOOL_USAGE;
        }
        if (arguments[0] == "help" || arguments[0] == "--help")
        {
            out << K_USAGE;
            return K_TOOL_OK;
        }
        const std::vector<std::string> rest(arguments.begin() + 1, arguments.end());
        for (const NamedCommand& command : K_COMMANDS)
        {
            if (command.name == arguments[0])
                return command.run(rest, out, err);
        }
        err << "unknown command \"" << arguments[0] << "\"\n" << K_USAGE;
        return K_TOOL_USAGE;
    }
} // namespace SushiProfiler
