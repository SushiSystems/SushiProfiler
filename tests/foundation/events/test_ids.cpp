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
