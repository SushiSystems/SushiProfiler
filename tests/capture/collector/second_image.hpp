/**************************************************************************/
/* second_image.hpp                                                       */
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
 * @file second_image.hpp
 * @brief Declares the function a second shared image exports to record one zone.
 * @author Mustafa Garip
 * @see tests/capture/collector/README.md
 */

#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(SP_SECOND_IMAGE_EXPORTS)
        #define SP_SECOND_IMAGE_API __declspec(dllexport)
    #else
        #define SP_SECOND_IMAGE_API __declspec(dllimport)
    #endif
#else
    #define SP_SECOND_IMAGE_API __attribute__((visibility("default")))
#endif

namespace SushiProfilerTests
{
    /// @brief Records one zone named "second_image_zone" on the calling thread and returns.
    SP_SECOND_IMAGE_API void record_zone_in_second_image();
} // namespace SushiProfilerTests
