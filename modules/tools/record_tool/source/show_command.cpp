/**************************************************************************/
/* show_command.cpp                                                       */
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
 * @file show_command.cpp
 * @brief Runs the named analyses on one record and prints their tables.
 * @author Mustafa Garip
 * @see modules/tools/record_tool/README.md
 */

#include "record_commands.hpp"

#include <charconv>
#include <optional>

#include <SushiProfiler/analysis/analysis_catalog.hpp>
#include <SushiProfiler/analysis/analysis_runner.hpp>
#include <SushiProfiler/record_tool/record_tool.hpp>
#include <SushiProfiler/record_writers/terminal_writer.hpp>

#include "tool_arguments.hpp"

namespace SushiProfiler::Detail
{
    namespace
    {
        /// @brief The analyses `show` runs when none is named.
        const std::vector<std::string> K_DEFAULT_ANALYSES = {
            "breakdown", "distribution", "counters", "flows", "top_spans", "overhead"};
    } // namespace

    int show_command(const std::vector<std::string>& arguments, std::ostream& out,
                     std::ostream& err)
    {
        const ToolArguments parsed =
            parse_tool_arguments(arguments, {"analysis", "top", "group-by", "baseline"}, {});
        if (!parsed.error.empty() || parsed.positional.size() != 1U)
        {
            err << (parsed.error.empty() ? "show takes one record" : parsed.error) << '\n';
            return K_TOOL_USAGE;
        }
        int failure = K_TOOL_OK;
        const std::optional<RunRecord> record =
            read_record_file(parsed.positional[0], err, failure);
        if (!record)
            return failure;
        AnalysisOptions options;
        if (const auto top = parsed.options.find("top"); top != parsed.options.end())
        {
            const std::string& text = top->second.back();
            const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(),
                                                      options.top);
            if (error != std::errc() || end != text.data() + text.size() || options.top == 0U)
            {
                err << "--top takes a whole number above 0, not \"" << text << "\"\n";
                return K_TOOL_USAGE;
            }
        }
        if (const auto keys = parsed.options.find("group-by"); keys != parsed.options.end())
            options.group_by = keys->second;
        std::optional<RunRecord> baseline;
        if (const auto path = parsed.options.find("baseline"); path != parsed.options.end())
        {
            baseline = read_record_file(path->second.back(), err, failure);
            if (!baseline)
                return failure;
            options.baseline = &*baseline;
        }
        const auto named = parsed.options.find("analysis");
        const std::vector<std::string>& names =
            named == parsed.options.end() ? K_DEFAULT_ANALYSES : named->second;
        AnalysisCatalog catalog;
        register_builtin_analyses(catalog);
        const AnalysisRun run = AnalysisRunner(catalog).run(*record, names, options);
        if (!run.error.empty())
        {
            err << run.error << '\n';
            return K_TOOL_USAGE;
        }
        TerminalWriter writer(out, options.top);
        return writer.write(*record, run.tables) == WriteStatus::Written ? K_TOOL_OK
                                                                         : K_TOOL_FAILED;
    }
} // namespace SushiProfiler::Detail
