/**************************************************************************/
/* main.cpp                                                               */
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
 * @file main.cpp
 * @brief Runs the record tool on the command line's arguments.
 * @author Mustafa Garip
 * @see applications/sp_record/README.md
 */

#include <iostream>
#include <string>
#include <vector>

#include <SushiProfiler/record_tool/record_tool.hpp>

/// @brief Passes the arguments after the program name to the record tool.
int main(int argc, char** argv)
{
    const std::vector<std::string> arguments(argv + 1, argv + argc);
    return SushiProfiler::run_record_tool(arguments, std::cout, std::cerr);
}
