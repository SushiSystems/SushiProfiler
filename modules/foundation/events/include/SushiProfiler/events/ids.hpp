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
