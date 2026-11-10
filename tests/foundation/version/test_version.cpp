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

#include <array>
#include <charconv>
#include <string_view>

namespace
{
    /// @brief Reads @p text, three numbers joined by points, into major, minor and patch.
    std::array<int, 3> numbers_of(std::string_view text)
    {
        std::array<int, 3> numbers{};
        const char* cursor = text.data();
        const char* const end = text.data() + text.size();
        for (int& number : numbers)
        {
            cursor = std::from_chars(cursor, end, number).ptr + 1;
        }
        return numbers;
    }
} // namespace

TEST(Small_Version, TheStringReadsAsTheThreeNumbers)
{
    const std::array<int, 3> expected{SushiProfiler::K_VERSION_MAJOR, SushiProfiler::K_VERSION_MINOR,
                                      SushiProfiler::K_VERSION_PATCH};

    EXPECT_EQ(numbers_of(SushiProfiler::K_VERSION_STRING), expected);
}
