/**************************************************************************/
/* device_span_keys.cpp                                                   */
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
 * @file device_span_keys.cpp
 * @brief Builds the environment fact keys of a device track's ceilings.
 * @author Mustafa Garip
 * @see modules/record/record/README.md
 */

#include <SushiProfiler/record/device_span_keys.hpp>

namespace SushiProfiler::DeviceSpanKeys
{
    namespace
    {
        /// @brief Returns `device.<track>.<ceiling>`.
        std::string ceiling_fact(std::string_view track, std::string_view ceiling)
        {
            std::string key = "device.";
            key += track;
            key += '.';
            key += ceiling;
            return key;
        }
    } // namespace

    std::string peak_flops_fact(std::string_view track)
    {
        return ceiling_fact(track, "peak_flops");
    }

    std::string peak_flops_fact(std::string_view track, std::string_view dtype)
    {
        std::string key = ceiling_fact(track, "peak_flops");
        key += '.';
        key += dtype;
        return key;
    }

    std::string peak_bytes_per_s_fact(std::string_view track)
    {
        return ceiling_fact(track, "peak_bytes_per_s");
    }
} // namespace SushiProfiler::DeviceSpanKeys
