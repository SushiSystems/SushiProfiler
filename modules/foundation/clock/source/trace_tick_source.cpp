/**************************************************************************/
/* trace_tick_source.cpp                                                  */
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
 * @file trace_tick_source.cpp
 * @brief Reads the processor's counter or the steady clock for a collector's stamps.
 * @author Mustafa Garip
 * @see modules/foundation/clock/README.md
 */

#include <SushiProfiler/clock/trace_tick_source.hpp>

#include <SushiProfiler/clock/trace_clock.hpp>

namespace SushiProfiler
{
    TraceTickSource::TraceTickSource() noexcept : use_tsc_(TraceClock::ticks_are_invariant())
    {

    }

    std::uint64_t TraceTickSource::now() const noexcept
    {
        return use_tsc_ ? TraceClock::now() : TraceClock::steady_now_ns();
    }

    CalibrationPoint TraceTickSource::sample() const noexcept
    {
        if (use_tsc_)
            return TraceClock::sample();
        const std::uint64_t steady = TraceClock::steady_now_ns();
        return CalibrationPoint{steady, steady};
    }

    bool TraceTickSource::ticks_are_tsc() const noexcept
    {
        return use_tsc_;
    }
} // namespace SushiProfiler
