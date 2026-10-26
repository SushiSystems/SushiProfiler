/**************************************************************************/
/* test_SushiProfiler.cpp                                                 */
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
 * @file test_SushiProfiler.cpp
 * @brief Tests that the umbrella header gives the public names of the library.
 * @author Mustafa Garip
 * @see tests/tools/umbrella/README.md
 */

#include <SushiProfiler/SushiProfiler.hpp>

#include <gtest/gtest.h>

TEST(Small_SushiProfiler, UmbrellaNamesTheVersion)
{
    EXPECT_FALSE(SushiProfiler::K_VERSION_STRING.empty());
}
