/**************************************************************************/
/* manual_tick_source.cpp                                                 */
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
 * @file manual_tick_source.cpp
 * @brief Keeps the time of the in-memory tick source.
 * @author Mustafa Garip
 * @see modules/foundation/clock/README.md
 */

#include <SushiProfiler/clock/manual_tick_source.hpp>

namespace SushiProfiler
{
    ManualTickSource::ManualTickSource(std::uint64_t start_ns) noexcept : now_ns_(start_ns)
    {

    }

    void ManualTickSource::advance(std::uint64_t elapsed_ns) noexcept
    {
        now_ns_.fetch_add(elapsed_ns, std::memory_order_relaxed);
    }

    std::uint64_t ManualTickSource::now() const noexcept
    {
        return now_ns_.load(std::memory_order_relaxed);
    }

    CalibrationPoint ManualTickSource::sample() const noexcept
    {
        const std::uint64_t now_ns = now();
        return CalibrationPoint{now_ns, now_ns};
    }

    bool ManualTickSource::ticks_are_tsc() const noexcept
    {
        return false;
    }
} // namespace SushiProfiler
