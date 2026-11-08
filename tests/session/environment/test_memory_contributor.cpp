/**************************************************************************/
/* test_memory_contributor.cpp                                            */
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
 * @file test_memory_contributor.cpp
 * @brief Tests that the in-memory contributor states the facts it holds, the last value winning.
 * @author Mustafa Garip
 * @see tests/session/environment/README.md
 */

#include <SushiProfiler/environment/memory_contributor.hpp>

#include <SushiProfiler/environment/record_facts.hpp>
#include <SushiProfiler/record/period_record.hpp>

#include <gtest/gtest.h>

#include <vector>

using SushiProfiler::Fact;
using SushiProfiler::MemoryContributor;
using SushiProfiler::RecordFacts;

TEST(Small_MemoryContributor, StatesItsEnvironmentAndBuildFacts)
{
    MemoryContributor contributor;
    contributor.state_environment("device", "cpu");
    contributor.state_build("commit", "abc123");
    RecordFacts facts;

    contributor.contribute(facts);

    EXPECT_EQ(facts.environment, (std::vector<Fact>{Fact{"device", "cpu"}}));
    EXPECT_EQ(facts.build, (std::vector<Fact>{Fact{"commit", "abc123"}}));
}

TEST(Small_MemoryContributor, AFactStatedAgainKeepsItsLastValue)
{
    MemoryContributor contributor;
    contributor.state_environment("device", "cpu");
    contributor.state_environment("device", "gpu");
    RecordFacts facts;

    contributor.contribute(facts);

    EXPECT_EQ(facts.environment, (std::vector<Fact>{Fact{"device", "gpu"}}));
}
