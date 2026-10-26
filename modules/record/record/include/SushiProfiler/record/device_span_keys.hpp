/**************************************************************************/
/* device_span_keys.hpp                                                   */
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
 * @file device_span_keys.hpp
 * @brief Declares the attribute and fact keys that device sources and the roofline analysis share.
 * @author Mustafa Garip
 * @see modules/record/record/README.md
 */

#pragma once

#include <SushiProfiler/version/export.hpp>

#include <string>
#include <string_view>

namespace SushiProfiler::DeviceSpanKeys
{
    /// @brief Integer span attribute: the index of the graph node that ran.
    inline constexpr std::string_view K_NODE = "node";

    /// @brief Integer span attribute: which invocation of the graph the span belongs to.
    inline constexpr std::string_view K_INVOCATION = "invocation";

    /// @brief Name span attribute: the label of the graph whose plan ran the node.
    inline constexpr std::string_view K_GRAPH = "graph";

    /// @brief Name span attribute: the caller's label for the operation the node was recorded for.
    inline constexpr std::string_view K_ORIGIN = "origin";

    /// @brief Integer span attribute: the id of the operation the node performs.
    inline constexpr std::string_view K_OPERATION = "operation";

    /// @brief Integer span attribute: the work items the launch started.
    inline constexpr std::string_view K_LAUNCHED_WIDTH = "launched_width";

    /// @brief Integer span attribute: the work items that did useful work.
    inline constexpr std::string_view K_LIVE_WIDTH = "live_width";

    /// @brief Name span attribute: the element type the span computed in, such as `f32`.
    inline constexpr std::string_view K_DTYPE = "dtype";

    /// @brief Integer span attribute: the floating-point operations the span performed.
    inline constexpr std::string_view K_FLOPS = "flops";

    /// @brief Integer span attribute: the bytes the span moved to and from memory.
    inline constexpr std::string_view K_BYTES = "bytes";

    /**
     * @brief Returns the environment fact that holds the compute ceiling of @p track.
     *
     * The fact is `device.<track>.peak_flops`; its value is a positive integer or real in text,
     * the track's peak in FLOP/s. It is the ceiling of spans with no `dtype` or no ceiling of
     * their own.
     */
    [[nodiscard]] SUSHIPROFILER_API std::string peak_flops_fact(std::string_view track);

    /**
     * @brief Returns the environment fact that holds the compute ceiling of @p track for @p dtype.
     *
     * The fact is `device.<track>.peak_flops.<dtype>`, a positive number in text, in FLOP/s.
     */
    [[nodiscard]] SUSHIPROFILER_API std::string peak_flops_fact(std::string_view track,
                                                                std::string_view dtype);

    /**
     * @brief Returns the environment fact that holds the memory ceiling of @p track.
     *
     * The fact is `device.<track>.peak_bytes_per_s`; its value is a positive integer or real in
     * text, the track's peak memory bandwidth in bytes per second.
     */
    [[nodiscard]] SUSHIPROFILER_API std::string peak_bytes_per_s_fact(std::string_view track);
} // namespace SushiProfiler::DeviceSpanKeys
