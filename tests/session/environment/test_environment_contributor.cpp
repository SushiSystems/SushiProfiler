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
 * @file test_environment_contributor.cpp
 * @brief Tests that gathering starts from host and build facts and runs contributors in order.
 * @author Mustafa Garip
 * @see tests/session/environment/README.md
 */

#include <SushiProfiler/environment/environment_contributor.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>

using SushiProfiler::EnvironmentContributor;
using SushiProfiler::Fact;
using SushiProfiler::RecordFacts;

namespace
{
    /// @brief Sets one environment fact and one build fact.
    class LibraryFacts final : public EnvironmentContributor
    {
        public:
            /// @brief Builds a contributor of `library` = @p library and `build_type` = @p build.
            LibraryFacts(std::string library, std::string build)
                : library_(std::move(library)), build_(std::move(build))
            {

            }

            /// @brief Sets `library` and `build_type`.
            void contribute(RecordFacts& facts) const override
            {
                SushiProfiler::set_fact(facts.environment, "library", library_);
                SushiProfiler::set_fact(facts.build, "build_type", build_);
            }

        private:
            std::string library_; /**< The `library` value. */
            std::string build_;   /**< The `build_type` value. */
    };

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
    const LibraryFacts first("sushiai v0.4", "debug");
    const LibraryFacts second("sushiai v0.5", "release");

    const RecordFacts facts = SushiProfiler::gather_facts({&first, &second});

    EXPECT_EQ(facts.environment.size(), SushiProfiler::host_facts().size() + 1U);
    EXPECT_EQ(value_of(facts.environment, "library"), "sushiai v0.5");
    EXPECT_EQ(value_of(facts.build, "build_type"), "release");
    EXPECT_EQ(facts.build.size(), 2U);
}
