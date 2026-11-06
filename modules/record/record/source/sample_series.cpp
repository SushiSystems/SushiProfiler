/**************************************************************************/
/* sample_series.cpp                                                      */
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
 * @file sample_series.cpp
 * @brief Compares sample series field by field.
 * @author Mustafa Garip
 * @see modules/record/record/README.md
 */

#include <SushiProfiler/record/sample_series.hpp>

#include <tuple>

namespace SushiProfiler
{
    bool SampleSeries::operator==(const SampleSeries& other) const
    {
        return std::tie(site, thread, time_ns, values, items, points) ==
               std::tie(other.site, other.thread, other.time_ns, other.values, other.items,
                        other.points);
    }
} // namespace SushiProfiler
