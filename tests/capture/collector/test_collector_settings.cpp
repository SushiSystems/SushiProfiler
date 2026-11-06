/**************************************************************************/
/* test_collector_settings.cpp                                            */
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
 * @file test_collector_settings.cpp
 * @brief Tests the collector's default settings.
 * @author Mustafa Garip
 * @see tests/capture/collector/README.md
 */

#include <SushiProfiler/collector/collector_settings.hpp>

#include <gtest/gtest.h>

using SushiProfiler::CollectorSettings;

TEST(Small_CollectorSettings, DefaultsKeepEverySiteInRingsOfSixtyFourKiloEvents)
{
    const CollectorSettings settings;

    EXPECT_EQ(settings.ring_events, 65536U);
    EXPECT_EQ(settings.filter.level, 3U);
    EXPECT_TRUE(settings.filter.groups.empty());
}
