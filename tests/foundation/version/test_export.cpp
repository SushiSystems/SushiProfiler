/**************************************************************************/
/* test_export.cpp                                                        */
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

/**
 * @file test_export.cpp
 * @brief Tests that the export header marks a declaration in the configuration the tests link.
 * @author Mustafa Garip
 * @see tests/foundation/version/README.md
 */

#include <SushiProfiler/version/export.hpp>

#include <gtest/gtest.h>

#include <cstring>

#define SP_TEST_STRINGIFY_EXPANDED(text) #text
#define SP_TEST_STRINGIFY(text) SP_TEST_STRINGIFY_EXPANDED(text)

TEST(Small_Export, TheSharedLibraryConsumerImportsItsSymbols)
{
    const char* const expanded = SP_TEST_STRINGIFY(SUSHIPROFILER_API);
#if defined(_WIN32)
    EXPECT_NE(std::strstr(expanded, "dllimport"), nullptr);
#else
    EXPECT_NE(std::strstr(expanded, "visibility"), nullptr);
#endif
}
