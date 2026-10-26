/**************************************************************************/
/* period_retention.cpp                                                   */
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
 * @file period_retention.cpp
 * @brief Erases the oldest periods of a record in batches, or exactly at the end.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#include <SushiProfiler/consumer/period_retention.hpp>

#include <cstddef>

namespace SushiProfiler
{
    void trim_periods(std::vector<PeriodRecord>& periods, std::optional<std::uint32_t> keep,
                      bool final_trim)
    {
        if (!keep.has_value())
            return;
        const std::size_t newest = *keep;
        if (newest == 0U)
        {
            periods.clear();
            return;
        }
        if (periods.size() <= newest)
            return;
        if (!final_trim && periods.size() < 2U * newest)
            return;
        periods.erase(periods.begin(),
                      periods.begin() + static_cast<std::ptrdiff_t>(periods.size() - newest));
    }
} // namespace SushiProfiler
