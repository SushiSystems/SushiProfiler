/**************************************************************************/
/* compile_collector_settings.cpp                                         */
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
 * @file compile_collector_settings.cpp
 * @brief Checks at compile time that the collector settings are a plain copyable value.
 * @author Mustafa Garip
 * @see tests/capture/collector/README.md
 */

#include <SushiProfiler/collector/collector_settings.hpp>

#include <cstddef>
#include <type_traits>
namespace
{
    using SushiProfiler::CollectorSettings;

    static_assert(std::is_default_constructible_v<CollectorSettings>, "settings need no argument");
    static_assert(std::is_copy_constructible_v<CollectorSettings>, "a session copies its settings");
    static_assert(std::is_same_v<decltype(CollectorSettings::ring_events), std::size_t>,
                  "ring sizes count events as a size");
} // namespace
