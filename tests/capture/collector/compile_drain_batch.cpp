/**************************************************************************/
/* compile_drain_batch.cpp                                                */
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
 * @file compile_drain_batch.cpp
 * @brief Checks at compile time that a drain is a movable value with no argument to build it.
 * @author Mustafa Garip
 * @see tests/capture/collector/README.md
 */

#include <SushiProfiler/collector/drain_batch.hpp>

#include <type_traits>
namespace
{
    using SushiProfiler::DrainBatch;
    using SushiProfiler::ThreadDrain;

    static_assert(std::is_default_constructible_v<ThreadDrain>, "an empty thread drain needs no argument");
    static_assert(std::is_default_constructible_v<DrainBatch>, "an empty batch needs no argument");
    static_assert(std::is_nothrow_move_constructible_v<DrainBatch>, "a batch moves its threads without throwing");
} // namespace
