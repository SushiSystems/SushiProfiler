/**************************************************************************/
/* sample_series.hpp                                                      */
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
 * @file sample_series.hpp
 * @brief Declares one site's samples on one thread within a period.
 * @author Mustafa Garip
 * @see modules/record/record/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstdint>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief Names where a flow point sits on its item's path.
    enum class FlowPoint : std::uint8_t
    {
        Begin, /**< The path starts. */
        Step,  /**< A point on the path. */
        End    /**< The path ends. */
    };

    /**
     * @brief Holds what one counter, value, instant or flow site recorded on one thread.
     *
     * Every series has `time_ns`. A counter or value series also has `values`, one per time; a
     * flow series has `items` and `points`; an instant series has times alone.
     */
    struct SUSHIPROFILER_API SampleSeries
    {
        SiteID site = 0U;                  /**< The recording site. */
        std::uint32_t thread = 0U;         /**< The recording thread's index. */
        std::vector<std::uint64_t> time_ns; /**< When each sample was taken. */
        std::vector<double> values;        /**< A counter's or value's samples. */
        std::vector<std::uint64_t> items;  /**< A flow point's item id. */
        std::vector<FlowPoint> points;     /**< A flow point's place on its path. */

        /// @brief Returns whether every field is equal.
        [[nodiscard]] bool operator==(const SampleSeries& other) const;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
