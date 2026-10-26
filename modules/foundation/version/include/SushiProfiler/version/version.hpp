/**************************************************************************/
/* version.hpp                                                            */
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
 * @file version.hpp
 * @brief Declares the version of the SushiProfiler library.
 * @author Mustafa Garip
 * @see modules/foundation/version/README.md
 */

#pragma once

#include <string_view>

namespace SushiProfiler
{
    /// @brief Major number of the library version.
    inline constexpr int K_VERSION_MAJOR = 0;

    /// @brief Minor number of the library version.
    inline constexpr int K_VERSION_MINOR = 1;

    /// @brief Patch number of the library version.
    inline constexpr int K_VERSION_PATCH = 0;

    /// @brief The library version as dotted numbers; the configure step compares it with the
    ///        version `project()` declares in CMakeLists.txt.
    inline constexpr std::string_view K_VERSION_STRING = "0.1.0";
} // namespace SushiProfiler
