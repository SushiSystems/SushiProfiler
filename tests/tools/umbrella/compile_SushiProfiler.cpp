/**************************************************************************/
/* compile_SushiProfiler.cpp                                              */
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

/**
 * @file compile_SushiProfiler.cpp
 * @brief Checks at compile time that the umbrella header alone gives a public name of each tier.
 * @author Mustafa Garip
 * @see tests/tools/umbrella/README.md
 */

#include <SushiProfiler/SushiProfiler.hpp>

#include <string_view>
#include <type_traits>

namespace
{
    static_assert(std::is_class_v<SushiProfiler::JsonValue>, "the umbrella gives the json tier");
    static_assert(std::is_class_v<SushiProfiler::TraceEvent>, "the umbrella gives the events");
    static_assert(std::is_class_v<SushiProfiler::Collector>, "the umbrella gives the collector");
    static_assert(std::is_class_v<SushiProfiler::ProfilerConfig>, "the umbrella gives the config");
    static_assert(std::is_class_v<SushiProfiler::RunRecord>, "the umbrella gives the record");
    static_assert(std::is_same_v<decltype(SushiProfiler::K_VERSION_STRING), const std::string_view>,
                  "the umbrella gives the version");
} // namespace
