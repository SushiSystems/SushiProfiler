/**************************************************************************/
/* test_ids.cpp                                                           */
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
 * @file test_ids.cpp
 * @brief Tests that the largest site id leaves the site word's top four bits to the kind.
 * @author Mustafa Garip
 * @see tests/foundation/events/README.md
 */

#include <SushiProfiler/events/ids.hpp>

#include <gtest/gtest.h>

TEST(Small_IDs, TheLargestSiteLeavesFourBitsForTheKind)
{
    EXPECT_EQ(SushiProfiler::K_MAX_SITE_ID, 0x0FFFFFFFU);
    EXPECT_EQ(SushiProfiler::K_MAX_SITE_ID >> 28U, 0U);
}
