/**************************************************************************/
/* test_chrome_trace_writer.cpp                                           */
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
 * @file test_chrome_trace_writer.cpp
 * @brief Tests that a streamed trace parses and holds rows, zones, samples, flows and spans.
 * @author Mustafa Garip
 * @see tests/session/stream_writers/README.md
 */

#include <SushiProfiler/stream_writers/chrome_trace_writer.hpp>

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <SushiProfiler/json/json_reader.hpp>
#include <SushiProfiler/sites/site.hpp>

#include "chrome_trace_fixture.hpp"

using SushiProfiler::ChromeTraceWriter;
using SushiProfiler::JsonReadResult;
using SushiProfiler::JsonValue;
using SushiProfiler::RunRecord;
using SushiProfiler::WriteStatus;
using SushiProfiler::ZoneRun;
using SushiProfilerTests::event_named;
using SushiProfilerTests::number;
using SushiProfilerTests::row_name;
using SushiProfilerTests::trace_instances;
using SushiProfilerTests::trace_period;
using SushiProfilerTests::trace_record;

namespace
{
    /// @brief Returns the trace's events after writing one period through a writer.
    std::vector<JsonValue> written_events(const std::string& path)
    {
        const RunRecord record = trace_record();
        const std::vector<ZoneRun> instances = trace_instances();
        ChromeTraceWriter writer(path);
        EXPECT_EQ(writer.begin(record), WriteStatus::Written);
        EXPECT_EQ(writer.write_period(record, trace_period(), instances), WriteStatus::Written);
        EXPECT_EQ(writer.end(record), WriteStatus::Written);
        std::stringstream text;
        {
            std::ifstream file(path, std::ios::binary);
            text << file.rdbuf();
        }
        std::remove(path.c_str());
        const JsonReadResult parsed = SushiProfiler::read_json(text.str());
        EXPECT_TRUE(parsed.ok()) << parsed.error.message;
        if (!parsed.ok())
            return {};
        EXPECT_EQ(*parsed.value->find("displayTimeUnit")->as_string(), "ns");
        return *parsed.value->find("traceEvents")->as_array();
    }
} // namespace

TEST(Medium_ChromeTraceWriter, WritesACounterSample)
{
    const std::vector<JsonValue> events =
        written_events(::testing::TempDir() + "sushiprofiler_counter.trace.json");

    const JsonValue* const counter = event_named(events, "C", "allocated (B)");
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(number(*counter->find("args"), "value"), 64.0);
}

TEST(Medium_ChromeTraceWriter, WritesAnInstant)
{
    const std::vector<JsonValue> events =
        written_events(::testing::TempDir() + "sushiprofiler_instant.trace.json");

    EXPECT_NE(event_named(events, "i", "underrun"), nullptr);
}

TEST(Medium_ChromeTraceWriter, WritesAFlowFromItsBeginToItsEnd)
{
    const std::vector<JsonValue> events =
        written_events(::testing::TempDir() + "sushiprofiler_flow.trace.json");

    const JsonValue* const begin = event_named(events, "s", "item");
    const JsonValue* const end = event_named(events, "f", "item");
    ASSERT_NE(begin, nullptr);
    ASSERT_NE(end, nullptr);
    EXPECT_EQ(number(*begin, "id"), 9.0);
    EXPECT_EQ(number(*end, "id"), 9.0);
    EXPECT_EQ(number(*end, "ts"), 2.9);
}

TEST(Medium_ChromeTraceWriter, WritesASpanOnItsDeviceTrackRow)
{
    const std::vector<JsonValue> events =
        written_events(::testing::TempDir() + "sushiprofiler_spans.trace.json");

    const JsonValue* const gemm = event_named(events, "X", "gemm");
    ASSERT_NE(gemm, nullptr);
    EXPECT_EQ(number(*gemm, "pid"), 2.0);
    EXPECT_EQ(number(*gemm, "dur"), 1.0);
    EXPECT_EQ(row_name(events, 2.0, 0.0), "queue 0");
}

TEST(Medium_ChromeTraceWriter, WritesASpansQueueTimeAndAttributesAsArguments)
{
    const std::vector<JsonValue> events =
        written_events(::testing::TempDir() + "sushiprofiler_args.trace.json");

    const JsonValue* const gemm = event_named(events, "X", "gemm");
    ASSERT_NE(gemm, nullptr);
    EXPECT_EQ(number(*gemm->find("args"), "bytes"), 4096.0);
    EXPECT_EQ(number(*gemm->find("args"), "queued_us"), 1.2);
}

TEST(Medium_ChromeTraceWriter, AnUnwritablePathIsReported)
{
    const RunRecord record = trace_record();
    ChromeTraceWriter writer(::testing::TempDir() + "missing_folder/none/trace.json");

    EXPECT_EQ(writer.begin(record), WriteStatus::PathUnwritable);
    EXPECT_EQ(writer.write_period(record, trace_period(), {}), WriteStatus::StreamFailed);
    EXPECT_EQ(writer.end(record), WriteStatus::StreamFailed);
}
