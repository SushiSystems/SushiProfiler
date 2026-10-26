/**************************************************************************/
/* period_retention.hpp                                                   */
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
 * @file period_retention.hpp
 * @brief Declares how many closed periods a run record keeps while it runs and at the end.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#pragma once

#include <SushiProfiler/record/period_record.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace SushiProfiler
{
    /**
     * @brief Trims @p periods to the newest @p keep, in batches while a run is going.
     *
     * An absent @p keep keeps every period; 0 keeps none. While running (`final_trim` false) the
     * list is trimmed only once it holds `2 * keep` periods, so erasing the front costs one move
     * per period kept, not per close; at the end (`final_trim` true) it is trimmed to exactly
     * @p keep.
     */
    SUSHIPROFILER_API void trim_periods(std::vector<PeriodRecord>& periods,
                                        std::optional<std::uint32_t> keep, bool final_trim);
} // namespace SushiProfiler
