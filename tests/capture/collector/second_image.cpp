/**************************************************************************/
/* second_image.cpp                                                       */
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
 * @file second_image.cpp
 * @brief Defines the function a second shared image exports to record one zone.
 * @author Mustafa Garip
 * @see tests/capture/collector/README.md
 */

#include "second_image.hpp"

#include <SushiProfiler/zones/macros.hpp>

namespace SushiProfilerTests
{
    void record_zone_in_second_image()
    {
        SP_ZONE("second_image_zone");
    }
} // namespace SushiProfilerTests
