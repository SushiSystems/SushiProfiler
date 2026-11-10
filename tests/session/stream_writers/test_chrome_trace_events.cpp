/**************************************************************************/
/* test_chrome_trace_events.cpp                                           */
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
 * @file test_chrome_trace_events.cpp
 * @brief Tests the rows the Chrome trace events of a period sit on, built with no file.
 * @author Mustafa Garip
 * @see tests/session/stream_writers/README.md
 */

#include <SushiProfiler/stream_writers/chrome_trace_events.hpp>

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <SushiProfiler/json/json_reader.hpp>

#include "chrome_trace_fixture.hpp"

using SushiProfiler::ChromeTraceEvents;
using SushiProfiler::JsonValue;
using SushiProfilerTests::event_named;
using SushiProfilerTests::number;
using SushiProfilerTests::row_name;
using SushiProfilerTests::trace_instances;
using SushiProfilerTests::trace_period;
using SushiProfilerTests::trace_record;

namespace
{
    /// @brief Returns the events of the opening and of one period, each parsed from its text.
    std::vector<JsonValue> built_events()
    {
        ChromeTraceEvents builder;
        std::vector<std::string> texts = builder.opening();
        const std::vector<std::string> period =
            builder.events_of(trace_record(), trace_period(), trace_instances());
        texts.insert(texts.end(), period.begin(), period.end());
        std::vector<JsonValue> events;
        for (const std::string& text : texts)
        {
            const SushiProfiler::JsonReadResult parsed = SushiProfiler::read_json(text);
            EXPECT_TRUE(parsed.ok()) << parsed.error.message;
            if (parsed.ok())
                events.push_back(*parsed.value);
        }
        return events;
    }
} // namespace

TEST(Small_ChromeTraceEvents, PlacesAZoneRunOnItsThreadsRow)
{
    const std::vector<JsonValue> events = built_events();

    const JsonValue* const conv = event_named(events, "X", "conv");
    ASSERT_NE(conv, nullptr);
    EXPECT_EQ(number(*conv, "ts"), 1.0);
    EXPECT_EQ(number(*conv, "dur"), 1.5);
    EXPECT_EQ(*conv->find("cat")->as_string(), "Model");
    EXPECT_EQ(row_name(events, 1.0, number(*conv, "tid")), "main");
}

TEST(Small_ChromeTraceEvents, PlacesALanesRunOnTheRowOfItsThreadAndLane)
{
    const std::vector<JsonValue> events = built_events();

    const JsonValue* const lane_zone = event_named(events, "X", "conv 3x3");
    ASSERT_NE(lane_zone, nullptr);
    EXPECT_EQ(row_name(events, 1.0, number(*lane_zone, "tid")), "main / voice");
}

TEST(Small_ChromeTraceEvents, PlacesThePeriodOnItsTimelineRow)
{
    const std::vector<JsonValue> events = built_events();

    const JsonValue* const period = event_named(events, "X", "step #4");
    ASSERT_NE(period, nullptr);
    EXPECT_EQ(number(*period, "dur"), 2.0);
    EXPECT_EQ(row_name(events, 1.0, number(*period, "tid")), "timeline step");
}
