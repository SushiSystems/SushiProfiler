/**************************************************************************/
/* test_source.cpp                                                        */
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
 * @file test_source.cpp
 * @brief Tests the Source port's promises on the in-memory source.
 * @author Mustafa Garip
 * @see tests/session/sources/README.md
 */

#include <SushiProfiler/sources/source.hpp>

#include <SushiProfiler/sources/memory_source.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using SushiProfiler::MemorySource;
using SushiProfiler::MemoryWork;
using SushiProfiler::NameTable;
using SushiProfiler::PeriodSpan;
using SushiProfiler::Source;
using SushiProfiler::SourceContext;
using SushiProfiler::SourceStatus;
using SushiProfiler::SpanSink;
using SushiProfiler::SpanTable;

TEST(Small_MemorySourceContract, AddsWorkThatStartedInsideThePeriodOnTheRecordsClock)
{
    NameTable names;
    std::vector<std::string> tracks;
    SourceContext context(names, tracks, 1'000U);
    MemorySource memory("test.queue");
    memory.queue(MemoryWork{"device", "kernel", 1'300U, 1'300U, 1'340U});
    Source& source = memory;
    ASSERT_TRUE(source.begin(context).ok());
    SpanTable period_three;
    SpanSink sink(period_three, names, tracks);

    source.collect(PeriodSpan{3U, 1'300U, 1'400U}, sink);

    ASSERT_EQ(period_three.size(), 1U);
    EXPECT_EQ(period_three.span(0U).start_ns, 300U);
    EXPECT_EQ(period_three.span(0U).end_ns, 340U);
    EXPECT_EQ(tracks, std::vector<std::string>{"device"});
}

TEST(Small_MemorySourceContract, DropsWorkThatStartedBeforeThePeriodAsLate)
{
    NameTable names;
    std::vector<std::string> tracks;
    SourceContext context(names, tracks, 0U);
    MemorySource memory("test.queue");
    memory.queue(MemoryWork{"device", "kernel", 90U, 90U, 120U});
    Source& source = memory;
    ASSERT_TRUE(source.begin(context).ok());
    SpanTable period;
    SpanSink sink(period, names, tracks);

    source.collect(PeriodSpan{1U, 100U, 200U}, sink);

    EXPECT_EQ(period.size(), 0U);
    EXPECT_EQ(sink.late(), 1U);
}

TEST(Small_MemorySourceContract, ASourceThatCannotStartSaysWhyAndAddsNoTrack)
{
    NameTable names;
    std::vector<std::string> tracks;
    SourceContext context(names, tracks, 0U);
    MemorySource memory("test.queue");
    memory.refuse_with("no device");

    const SourceStatus status = static_cast<Source&>(memory).begin(context);

    EXPECT_EQ(status.error, "no device");
    EXPECT_TRUE(tracks.empty());
}
