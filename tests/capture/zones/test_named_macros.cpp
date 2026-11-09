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
    /// @brief The level-0 macro tests that reach the process-wide collector or start a thread.
    class Medium_NamedMacros : public SushiProfilerTests::RecordingFixture
    {
    };

    static_assert(SP_PROFILE_LEVEL == 0, "this unit compiles the macros out");
} // namespace

TEST(Small_NamedMacros, ThreadNameDoesNotEvaluateItsArgumentAtLevelZero)
{
    int evaluated = 0;
    SP_THREAD_NAME((++evaluated, "never.set"));

    EXPECT_EQ(evaluated, 0);
}

TEST(Small_NamedMacros, NamedRecordsDoNotBuildTheirNameAtLevelZero)
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
}

TEST_F(Medium_NamedMacros, ThreadNameNamesNothingAtLevelZero)
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

TEST_F(Medium_NamedMacros, NamedRecordsWriteNothingAtLevelZero)
{
    SP_COUNTER_NAMED(std::string("level0.name"), "", Add, 1.0);
    SP_VALUE_NAMED(std::string("level0.name"), "", 1.0);

    EXPECT_TRUE(drain_mine().empty());
}
