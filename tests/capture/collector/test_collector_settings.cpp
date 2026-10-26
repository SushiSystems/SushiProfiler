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
