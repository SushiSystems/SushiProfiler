/**************************************************************************/
/* record_commands.hpp                                                    */
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
 * @file record_commands.hpp
 * @brief Declares the record tool's commands, one per source file.
 * @author Mustafa Garip
 * @see modules/tools/record_tool/README.md
 */

#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace SushiProfiler::Detail
{
    /// @brief Prints the analyses of one record as tables.
    [[nodiscard]] int show_command(const std::vector<std::string>& arguments, std::ostream& out,
                                   std::ostream& err);

    /// @brief Prints a baseline and a candidate record's comparison.
    [[nodiscard]] int compare_command(const std::vector<std::string>& arguments,
                                      std::ostream& out, std::ostream& err);

    /// @brief Prints each check's verdict on a record; fails when one fails.
    [[nodiscard]] int check_command(const std::vector<std::string>& arguments, std::ostream& out,
                                    std::ostream& err);

    /// @brief Reads a record and says whether it is valid.
    [[nodiscard]] int validate_command(const std::vector<std::string>& arguments,
                                       std::ostream& out, std::ostream& err);
} // namespace SushiProfiler::Detail
