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
