/**************************************************************************/
/* test_named_macros.cpp                                                 */
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
 * @file test_named_macros.cpp
 * @brief Tests that the macros compile to nothing when SP_PROFILE_LEVEL is 0.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#define SP_PROFILE_LEVEL 0

#include <SushiProfiler/zones/named_macros.hpp>

#include "recording_fixture.hpp"

#include <gtest/gtest.h>

#include <string>
#include <thread>

using SushiProfiler::Collector;

namespace
{
    /// @brief The level-0 macro tests.
    class Small_NamedMacros : public SushiProfilerTests::RecordingFixture
    {
    };

    static_assert(SP_PROFILE_LEVEL == 0, "this unit compiles the macros out");
} // namespace

TEST_F(Small_NamedMacros, ThreadNameIsCompiledOutAtLevelZero)
{
    std::thread(
        []
        {
            SP_THREAD_NAME("never.set");
            SushiProfiler::DrainBatch batch;
            Collector::instance().drain(batch);
            for (const SushiProfiler::ThreadDrain& drain : batch.threads)
                EXPECT_NE(drain.name, "never.set");
        })
        .join();
}

TEST_F(Small_NamedMacros, NamedRecordsAreCompiledOutAtLevelZero)
{
    int built = 0;
    const auto build = [&built]
    {
        ++built;
        return std::string("level0.name");
    };
    SP_COUNTER_NAMED(build(), "", Add, 1.0);
    SP_VALUE_NAMED(build(), "", 1.0);

    EXPECT_EQ(built, 0);
    EXPECT_TRUE(drain_mine().empty());
}
