/**************************************************************************/
/* ids.hpp                                                                */
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
 * @file ids.hpp
 * @brief Declares the ids that name a site and an interned string.
 * @author Mustafa Garip
 * @see modules/foundation/events/README.md
 */

#pragma once

#include <cstdint>

namespace SushiProfiler
{
    /// @brief Names one registered site; 0 is no site.
    using SiteID = std::uint32_t;

    /// @brief Names one interned string; 0 is none.
    using NameID = std::uint32_t;

    /// @brief The largest site id, since a site word keeps its top four bits for the kind.
    inline constexpr SiteID K_MAX_SITE_ID = (SiteID{1U} << 28U) - 1U;
} // namespace SushiProfiler
