/**************************************************************************/
/* consumer.cpp                                                           */
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
 * @file consumer.cpp
 * @brief Reads the version through an installed SushiProfiler target and fails unless it matches.
 * @author Mustafa Garip
 * @see tests/package/README.md
 */

#include <SushiProfiler/SushiProfiler.hpp>

#include <cstdio>
#include <string_view>

/**
 * @brief Compares the version the library reports with the one the consumer was built for.
 * @return 0 when they match, 1 otherwise.
 */
int main()
{
    const std::string_view reported = SushiProfiler::K_VERSION_STRING;
    if (reported != CONSUMER_EXPECTED_VERSION)
    {
        std::fprintf(stderr, "consumer: the library reports %.*s, expected %s\n",
                     static_cast<int>(reported.size()), reported.data(),
                     CONSUMER_EXPECTED_VERSION);
        return 1;
    }
    std::fprintf(stdout, "consumer: version %s\n", CONSUMER_EXPECTED_VERSION);
    return 0;
}
