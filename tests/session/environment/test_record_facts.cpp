/**************************************************************************/
/* test_record_facts.cpp                                                  */
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
 * @file test_record_facts.cpp
 * @brief Tests setting a fact and the names and shapes of the host and build facts.
 * @author Mustafa Garip
 * @see tests/session/environment/README.md
 */

#include <SushiProfiler/environment/record_facts.hpp>

#include <gtest/gtest.h>

#include <string>

using SushiProfiler::Fact;

TEST(Small_RecordFacts, SetFactAddsThenReplacesInPlace)
{
    std::vector<Fact> facts = {{"os", "Linux"}};

    SushiProfiler::set_fact(facts, "commit", "abc123");
    SushiProfiler::set_fact(facts, "os", "Windows");

    EXPECT_EQ(facts, (std::vector<Fact>{{"os", "Windows"}, {"commit", "abc123"}}));
}

TEST(Small_RecordFacts, HostFactsNameTheSystemCPUAndThreads)
{
    const std::vector<Fact> facts = SushiProfiler::host_facts();

    ASSERT_EQ(facts.size(), 3U);
    EXPECT_EQ(facts[0].name, "os");
#if defined(_WIN32)
    EXPECT_EQ(facts[0].value, "Windows");
#elif defined(__linux__)
    EXPECT_EQ(facts[0].value, "Linux");
#endif
    EXPECT_EQ(facts[1].name, "cpu");
    EXPECT_FALSE(facts[1].value.empty());
    EXPECT_NE(facts[1].value.front(), ' ');
    EXPECT_NE(facts[1].value.back(), ' ');
    EXPECT_EQ(facts[2].name, "hardware_threads");
    EXPECT_EQ(facts[2].value.find_first_not_of("0123456789"), std::string::npos);
}

TEST(Small_RecordFacts, BuildFactsNameTheCompilerAndBuildType)
{
    const std::vector<Fact> facts = SushiProfiler::library_build_facts();

    ASSERT_EQ(facts.size(), 2U);
    EXPECT_EQ(facts[0].name, "compiler");
    EXPECT_NE(facts[0].value.find(' '), std::string::npos);
    EXPECT_EQ(facts[1].name, "build_type");
    EXPECT_TRUE(facts[1].value == "debug" || facts[1].value == "release");
}
