/**************************************************************************/
/* test_source_context.cpp                                                */
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
 * @file test_source_context.cpp
 * @brief Tests interning through the context, adding tracks once each and the time origin.
 * @author Mustafa Garip
 * @see tests/session/sources/README.md
 */

#include <SushiProfiler/sources/source_context.hpp>

#include <gtest/gtest.h>

using SushiProfiler::NameTable;
using SushiProfiler::SourceContext;

TEST(Small_SourceContext, InternsIntoTheSessionNames)
{
    NameTable names;
    std::vector<std::string> tracks;
    SourceContext context(names, tracks, 0U);

    const auto id = context.intern("kernel");

    EXPECT_EQ(names.text(id), "kernel");
    EXPECT_EQ(context.intern("kernel"), id);
}

TEST(Small_SourceContext, AddsATrackOnceAndKeepsItsID)
{
    NameTable names;
    std::vector<std::string> tracks = {"cpu"};
    SourceContext context(names, tracks, 0U);

    EXPECT_EQ(context.add_track("gpu.queue0"), 1U);
    EXPECT_EQ(context.add_track("gpu.queue1"), 2U);
    EXPECT_EQ(context.add_track("gpu.queue0"), 1U);
    EXPECT_EQ(tracks, (std::vector<std::string>{"cpu", "gpu.queue0", "gpu.queue1"}));
}

TEST(Small_SourceContext, CountsTimeFromTheOrigin)
{
    NameTable names;
    std::vector<std::string> tracks;
    const SourceContext context(names, tracks, 1'000U);

    EXPECT_EQ(context.to_record_ns(1'250U), 250U);
    EXPECT_EQ(context.to_record_ns(1'000U), 0U);
    EXPECT_EQ(context.to_record_ns(400U), 0U);
}
