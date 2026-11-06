/**************************************************************************/
/* drain_batch.hpp                                                        */
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
 * @file drain_batch.hpp
 * @brief Declares what one drain of the collector hands its consumer.
 * @author Mustafa Garip
 * @see modules/capture/collector/README.md
 */

#pragma once

#include <SushiProfiler/clock/calibration.hpp>
#include <SushiProfiler/events/trace_event.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace SushiProfiler
{
    /// @brief One thread's share of a drain.
    struct ThreadDrain
    {
        std::uint32_t thread_index = 0U; /**< The thread's index, from 0, never reused. */
        std::string name;                /**< The thread's name, or empty. */
        std::vector<TraceEvent> events;  /**< Its events since the last drain, oldest first. */
        std::uint64_t dropped = 0U;      /**< Events its full ring dropped since the last drain. */
        double fill = 0.0;               /**< How full its ring was when drained. */
        bool retired = false;            /**< Whether the thread has exited; its last drain. */
    };

    /// @brief Everything one drain moved out of the rings.
    struct DrainBatch
    {
        CalibrationPoint point;          /**< A calibration point taken during the drain. */
        bool ticks_are_tsc = false;      /**< Whether event ticks are TSC, else steady ns. */
        std::vector<ThreadDrain> threads; /**< One entry per registered thread. */
    };
} // namespace SushiProfiler
