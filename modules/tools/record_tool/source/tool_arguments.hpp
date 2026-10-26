/**************************************************************************/
/* tool_arguments.hpp                                                     */
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
 * @file tool_arguments.hpp
 * @brief Declares the record tool's argument parser and its shared file readers.
 * @author Mustafa Garip
 * @see modules/tools/record_tool/README.md
 */

#pragma once

#include <SushiProfiler/record/run_record.hpp>

#include <cstddef>
#include <initializer_list>
#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace SushiProfiler::Detail
{
    /// @brief One command's arguments: its positional words, its options and its flags.
    struct ToolArguments
    {
        std::vector<std::string> positional;                     /**< Words in order. */
        std::map<std::string, std::vector<std::string>> options; /**< `--name value`, by name. */
        std::set<std::string> flags;                             /**< `--name` alone. */
        std::string error;                                       /**< The refusal, or empty. */
    };

    /**
     * @brief Splits @p arguments into positional words, options with a value and flags.
     *
     * An option is one of @p valued and takes the next word; a flag is one of @p flagged. Any
     * other word that starts with `--` is refused.
     */
    [[nodiscard]] ToolArguments parse_tool_arguments(
        const std::vector<std::string>& arguments, std::initializer_list<std::string_view> valued,
        std::initializer_list<std::string_view> flagged);

    /// @brief Returns the text of the file at @p path, or nothing when it cannot be read.
    [[nodiscard]] std::optional<std::string> read_text_file(const std::string& path);

    /**
     * @brief Reads the record at @p path, writing why not to @p err.
     * @param failure Set to K_TOOL_USAGE when the file cannot be read, else K_TOOL_FAILED.
     */
    [[nodiscard]] std::optional<RunRecord> read_record_file(const std::string& path,
                                                            std::ostream& err, int& failure);
} // namespace SushiProfiler::Detail
