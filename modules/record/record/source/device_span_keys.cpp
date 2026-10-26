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
/* Licensed under the PolyForm Noncommercial License 1.0.0 (the           */
/* "License"); you may not use this file except in compliance with the    */
/* License. You may obtain a copy of the License at                       */
/*                                                                        */
/*     https://polyformproject.org/licenses/noncommercial/1.0.0           */
/*                                                                        */
/* Noncommercial use is free. Commercial use requires a separate licence  */
/* from Sushi Systems; see COMMERCIAL.md. The software is provided        */
/* "as is", without warranty of any kind.                                 */
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
