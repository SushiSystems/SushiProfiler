/**************************************************************************/
/* test_device_span_keys.cpp                                              */
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
 * @file test_device_span_keys.cpp
 * @brief Tests the shared span attribute keys and the ceiling fact key builders.
 * @author Mustafa Garip
 * @see tests/record/record/README.md
 */

#include <SushiProfiler/record/device_span_keys.hpp>

#include <gtest/gtest.h>

namespace Keys = SushiProfiler::DeviceSpanKeys;

TEST(Small_DeviceSpanKeys, TheAttributeKeysAreTheWordsTheSourcesWrite)
{
    EXPECT_EQ(Keys::K_NODE, "node");
    EXPECT_EQ(Keys::K_GRAPH, "graph");
    EXPECT_EQ(Keys::K_ORIGIN, "origin");
    EXPECT_EQ(Keys::K_INVOCATION, "invocation");
    EXPECT_EQ(Keys::K_OPERATION, "operation");
    EXPECT_EQ(Keys::K_LAUNCHED_WIDTH, "launched_width");
    EXPECT_EQ(Keys::K_LIVE_WIDTH, "live_width");
    EXPECT_EQ(Keys::K_DTYPE, "dtype");
    EXPECT_EQ(Keys::K_FLOPS, "flops");
    EXPECT_EQ(Keys::K_BYTES, "bytes");
}

TEST(Small_DeviceSpanKeys, TheCeilingFactsNameTheTrack)
{
    EXPECT_EQ(Keys::peak_flops_fact("queue 0"), "device.queue 0.peak_flops");
    EXPECT_EQ(Keys::peak_bytes_per_s_fact("queue 0"), "device.queue 0.peak_bytes_per_s");
}

TEST(Small_DeviceSpanKeys, ThePerTypeComputeCeilingAppendsTheType)
{
    EXPECT_EQ(Keys::peak_flops_fact("queue 0", "f16"), "device.queue 0.peak_flops.f16");
}

TEST(Small_DeviceSpanKeys, AnEmptyTrackLeavesTheSeparatorsInPlace)
{
    EXPECT_EQ(Keys::peak_flops_fact(""), "device..peak_flops");
}
