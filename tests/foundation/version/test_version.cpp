/**************************************************************************/
/* test_version.cpp                                                       */
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
 * @file test_version.cpp
 * @brief Tests that the version string and the version numbers agree.
 * @author Mustafa Garip
 * @see tests/foundation/version/README.md
 */

#include <SushiProfiler/version/version.hpp>

#include <gtest/gtest.h>

#include <string>

TEST(Small_Version, StringIsTheDottedNumbers)
{
    const std::string dotted = std::to_string(SushiProfiler::K_VERSION_MAJOR) + "." +
                               std::to_string(SushiProfiler::K_VERSION_MINOR) + "." +
                               std::to_string(SushiProfiler::K_VERSION_PATCH);

    EXPECT_EQ(dotted, std::string(SushiProfiler::K_VERSION_STRING));
}
