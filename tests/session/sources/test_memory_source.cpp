/**************************************************************************/
/* test_memory_source.cpp                                                 */
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
 * @file test_memory_source.cpp
 * @brief Tests that the in-memory source keeps later work, states its facts and notes its calls.
 * @author Mustafa Garip
 * @see tests/session/sources/README.md
 */

#include <SushiProfiler/sources/memory_source.hpp>

#include <SushiProfiler/record/period_record.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using SushiProfiler::Fact;
using SushiProfiler::MemorySource;
using SushiProfiler::MemoryWork;
using SushiProfiler::NameTable;
using SushiProfiler::PeriodSpan;
using SushiProfiler::SourceContext;
using SushiProfiler::SpanSink;
using SushiProfiler::SpanTable;

namespace
{
    /// @brief Holds the names, tracks and context a started MemorySource collects through.
    class Small_MemorySource : public ::testing::Test
    {
        protected:
            NameTable names_;                                    /**< The session's names. */
            std::vector<std::string> tracks_;                    /**< The record's tracks. */
            SourceContext context_{names_, tracks_, 0U};         /**< The context begin() gets. */
            MemorySource source_{"test.memory", 5'000'000U};     /**< The source under test. */
    };
} // namespace

TEST_F(Small_MemorySource, KeepsWorkThatStartsAfterThePeriodForALaterOne)
{
    source_.queue(MemoryWork{"device", "kernel", 250U, 250U, 260U});
    ASSERT_TRUE(source_.begin(context_).ok());
    SpanTable first;
    SpanSink first_sink(first, names_, tracks_);
    source_.collect(PeriodSpan{1U, 100U, 200U}, first_sink);
    SpanTable second;
    SpanSink second_sink(second, names_, tracks_);

    source_.collect(PeriodSpan{2U, 200U, 300U}, second_sink);

    EXPECT_EQ(first.size(), 0U);
    ASSERT_EQ(second.size(), 1U);
    EXPECT_EQ(second.span(0U).start_ns, 250U);
}

TEST_F(Small_MemorySource, StatesItsFactsOnThePeriod)
{
    source_.state_fact("layer", "graph");
    ASSERT_TRUE(source_.begin(context_).ok());
    SpanTable period;
    std::vector<Fact> facts;
    SpanSink sink(period, facts, names_, tracks_);

    source_.collect(PeriodSpan{1U, 100U, 200U}, sink);

    EXPECT_EQ(facts, (std::vector<Fact>{Fact{"layer", "graph"}}));
}

TEST_F(Small_MemorySource, StatesTheDelayItWasGiven)
{
    EXPECT_EQ(source_.max_delay_ns(), 5'000'000U);
}

TEST_F(Small_MemorySource, NotesThePeriodsCollectedAndTheEnd)
{
    ASSERT_TRUE(source_.begin(context_).ok());
    SpanTable period;
    SpanSink sink(period, names_, tracks_);
    source_.collect(PeriodSpan{4U, 400U, 500U}, sink);

    source_.end();

    ASSERT_EQ(source_.collected().size(), 1U);
    EXPECT_EQ(source_.collected()[0].index, 4U);
    EXPECT_EQ(source_.ends(), 1U);
}
