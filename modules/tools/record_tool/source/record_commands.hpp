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
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
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
