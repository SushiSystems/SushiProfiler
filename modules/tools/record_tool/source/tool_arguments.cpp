/**************************************************************************/
/* tool_arguments.cpp                                                     */
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
 * @file tool_arguments.cpp
 * @brief Splits the tool's arguments and reads the files its commands name.
 * @author Mustafa Garip
 * @see modules/tools/record_tool/README.md
 */

#include "tool_arguments.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

#include <SushiProfiler/record_json/record_json.hpp>
#include <SushiProfiler/record_tool/record_tool.hpp>

namespace SushiProfiler::Detail
{
    namespace
    {
        /// @brief Returns whether @p words holds @p word.
        bool holds(std::initializer_list<std::string_view> words, std::string_view word)
        {
            return std::find(words.begin(), words.end(), word) != words.end();
        }
    } // namespace

    ToolArguments parse_tool_arguments(const std::vector<std::string>& arguments,
                                       std::initializer_list<std::string_view> valued,
                                       std::initializer_list<std::string_view> flagged)
    {
        ToolArguments parsed;
        for (std::size_t index = 0U; index < arguments.size(); ++index)
        {
            const std::string& word = arguments[index];
            if (word.rfind("--", 0U) != 0U)
            {
                parsed.positional.push_back(word);
                continue;
            }
            const std::string name = word.substr(2U);
            if (holds(flagged, name))
            {
                parsed.flags.insert(name);
                continue;
            }
            if (!holds(valued, name))
            {
                parsed.error = "unknown option " + word;
                return parsed;
            }
            if (index + 1U == arguments.size())
            {
                parsed.error = word + " needs a value";
                return parsed;
            }
            parsed.options[name].push_back(arguments[++index]);
        }
        return parsed;
    }

    std::optional<std::string> read_text_file(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return std::nullopt;
        std::ostringstream text;
        text << file.rdbuf();
        if (!file && !file.eof())
            return std::nullopt;
        return text.str();
    }

    std::optional<RunRecord> read_record_file(const std::string& path, std::ostream& err,
                                              int& failure)
    {
        const std::optional<std::string> text = read_text_file(path);
        if (!text)
        {
            err << path << ": could not be read\n";
            failure = K_TOOL_USAGE;
            return std::nullopt;
        }
        RecordReadResult read = read_record(*text);
        if (!read.ok())
        {
            err << path << ": " << read.error << '\n';
            failure = K_TOOL_FAILED;
            return std::nullopt;
        }
        return std::move(read.record);
    }
} // namespace SushiProfiler::Detail
