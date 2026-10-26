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
