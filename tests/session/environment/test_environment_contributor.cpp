/**************************************************************************/
/* test_environment_contributor.cpp                                       */
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
 * @file test_environment_contributor.cpp
 * @brief Tests that gathering starts from host and build facts and runs contributors in order.
 * @author Mustafa Garip
 * @see tests/session/environment/README.md
 */

#include <SushiProfiler/environment/environment_contributor.hpp>

#include <SushiProfiler/environment/memory_contributor.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using SushiProfiler::Fact;
using SushiProfiler::MemoryContributor;
using SushiProfiler::RecordFacts;

namespace
{
    /// @brief Returns a contributor of `library` = @p library and `build_type` = @p build.
    MemoryContributor library_facts(const std::string& library, const std::string& build)
    {
        MemoryContributor contributor;
        contributor.state_environment("library", library);
        contributor.state_build("build_type", build);
        return contributor;
    }

    /// @brief Returns the value of @p name in @p facts, or an empty string.
    std::string value_of(const std::vector<Fact>& facts, const std::string& name)
    {
        for (const Fact& fact : facts)
        {
            if (fact.name == name)
                return fact.value;
        }
        return std::string();
    }
} // namespace

TEST(Small_EnvironmentContributor, StartsFromTheHostAndTheLibraryBuild)
{
    const RecordFacts facts = SushiProfiler::gather_facts({});

    EXPECT_EQ(facts.environment, SushiProfiler::host_facts());
    EXPECT_EQ(facts.build, SushiProfiler::library_build_facts());
}

TEST(Small_EnvironmentContributor, AppliesContributorsInOrderAndTheLastWins)
{
    const MemoryContributor first = library_facts("sushiai v0.4", "debug");
    const MemoryContributor second = library_facts("sushiai v0.5", "release");

    const RecordFacts facts = SushiProfiler::gather_facts({&first, &second});

    EXPECT_EQ(facts.environment.size(), SushiProfiler::host_facts().size() + 1U);
    EXPECT_EQ(value_of(facts.environment, "library"), "sushiai v0.5");
    EXPECT_EQ(value_of(facts.build, "build_type"), "release");
    EXPECT_EQ(facts.build.size(), 2U);
}
