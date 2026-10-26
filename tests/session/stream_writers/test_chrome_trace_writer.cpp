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
 * @file test_chrome_trace_writer.cpp
 * @brief Tests that a streamed trace parses and holds rows, zones, samples, flows and spans.
 * @author Mustafa Garip
 * @see tests/session/stream_writers/README.md
 */

#include <SushiProfiler/stream_writers/chrome_trace_writer.hpp>

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

#include <SushiProfiler/json/json_reader.hpp>
#include <SushiProfiler/sites/site.hpp>

using SushiProfiler::ChromeTraceWriter;
using SushiProfiler::FlowPoint;
using SushiProfiler::JsonReadResult;
using SushiProfiler::JsonValue;
using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::SampleSeries;
using SushiProfiler::SiteRecord;
using SushiProfiler::SiteRole;
using SushiProfiler::Span;
using SushiProfiler::ThreadRecord;
using SushiProfiler::WriteStatus;
using SushiProfiler::ZoneRun;

namespace
{
    /// @brief Returns a site of @p role named @p name with @p group and @p unit.
    SiteRecord site_of(SiteRole role, std::string name, std::string group = "",
                       std::string unit = "")
    {
        SiteRecord site;
        site.role = role;
        site.name = std::move(name);
        site.group = std::move(group);
        site.unit = std::move(unit);
        return site;
    }

    /// @brief Returns a run with a zone, counter, flow and instant site, one thread and one track.
    RunRecord trace_record()
    {
        RunRecord record;
        record.names = {"", "step", "voice", "gemm", "bytes", "conv 3x3"};
        record.sites = {SiteRecord{}, site_of(SiteRole::Zone, "conv", "Model"),
                        site_of(SiteRole::Counter, "allocated", "", "B"),
                        site_of(SiteRole::Flow, "item"), site_of(SiteRole::Instant, "underrun")};
        record.tracks = {"queue 0"};
        record.threads = {ThreadRecord{0U, "main", 0U, 0U, 0U}};
        return record;
    }

    /// @brief Returns period 4 of `step` from 1 to 3 us with samples and one span.
    PeriodRecord trace_period()
    {
        PeriodRecord period;
        period.index = 4U;
        period.timeline = 1U;
        period.begin_ns = 1'000U;
        period.end_ns = 3'000U;
        SampleSeries counter;
        counter.site = 2U;
        counter.time_ns = {1'500U};
        counter.values = {64.0};
        SampleSeries flow;
        flow.site = 3U;
        flow.time_ns = {1'100U, 2'900U};
        flow.items = {9U, 9U};
        flow.points = {FlowPoint::Begin, FlowPoint::End};
        SampleSeries instant;
        instant.site = 4U;
        instant.time_ns = {2'000U};
        period.samples = {counter, flow, instant};
        Span span;
        span.queued_ns = 1'200U;
        span.start_ns = 1'400U;
        span.end_ns = 2'400U;
        span.name = 3U;
        const std::size_t row = period.spans.append(span);
        period.spans.set_integer(row, 4U, 4096);
        return period;
    }

    /// @brief Returns the trace's events after writing one period through a writer.
    std::vector<JsonValue> written_events(const std::string& path)
    {
        const RunRecord record = trace_record();
        const std::vector<ZoneRun> instances = {
            ZoneRun{0U, 0U, 1U, 0U, 1'000U, 2'500U, 0U},
            ZoneRun{0U, 2U, 1U, 5U, 1'200U, 1'300U, 0U}};
        ChromeTraceWriter writer(path);
        EXPECT_EQ(writer.begin(record), WriteStatus::Written);
        EXPECT_EQ(writer.write_period(record, trace_period(), instances), WriteStatus::Written);
        EXPECT_EQ(writer.end(record), WriteStatus::Written);
        std::ifstream file(path, std::ios::binary);
        std::stringstream text;
        text << file.rdbuf();
        const JsonReadResult parsed = SushiProfiler::read_json(text.str());
        EXPECT_TRUE(parsed.ok()) << parsed.error.message;
        if (!parsed.ok())
            return {};
        EXPECT_EQ(*parsed.value->find("displayTimeUnit")->as_string(), "ns");
        return *parsed.value->find("traceEvents")->as_array();
    }

    /// @brief Returns the first event of phase @p phase named @p name, or null.
    const JsonValue* event_named(const std::vector<JsonValue>& events, const std::string& phase,
                                 const std::string& name)
    {
        for (const JsonValue& event : events)
        {
            if (*event.find("ph")->as_string() == phase && *event.find("name")->as_string() == name)
                return &event;
        }
        return nullptr;
    }

    /// @brief Returns member @p key of @p event as a number.
    double number(const JsonValue& event, const char* key)
    {
        double value = 0.0;
        EXPECT_TRUE(event.find(key)->to_double(value)) << key;
        return value;
    }

    /// @brief Returns the row name a metadata event gives @p pid and @p tid, or an empty string.
    std::string row_name(const std::vector<JsonValue>& events, double pid, double tid)
    {
        for (const JsonValue& event : events)
        {
            if (*event.find("name")->as_string() == "thread_name" &&
                number(event, "pid") == pid && number(event, "tid") == tid)
                return *event.find("args")->find("name")->as_string();
        }
        return std::string();
    }
} // namespace

TEST(Medium_ChromeTraceWriter, WritesZonesOnThreadAndLaneRows)
{
    const std::vector<JsonValue> events =
        written_events(::testing::TempDir() + "sushiprofiler_zones.trace.json");

    const JsonValue* const conv = event_named(events, "X", "conv");
    ASSERT_NE(conv, nullptr);
    EXPECT_EQ(number(*conv, "ts"), 1.0);
    EXPECT_EQ(number(*conv, "dur"), 1.5);
    EXPECT_EQ(*conv->find("cat")->as_string(), "Model");
    EXPECT_EQ(row_name(events, 1.0, number(*conv, "tid")), "main");
    const JsonValue* const lane_zone = event_named(events, "X", "conv 3x3");
    ASSERT_NE(lane_zone, nullptr);
    EXPECT_EQ(row_name(events, 1.0, number(*lane_zone, "tid")), "main / voice");
}

TEST(Medium_ChromeTraceWriter, WritesThePeriodOnItsTimelineRow)
{
    const std::vector<JsonValue> events =
        written_events(::testing::TempDir() + "sushiprofiler_period.trace.json");

    const JsonValue* const period = event_named(events, "X", "step #4");
    ASSERT_NE(period, nullptr);
    EXPECT_EQ(number(*period, "dur"), 2.0);
    EXPECT_EQ(row_name(events, 1.0, number(*period, "tid")), "timeline step");
}

TEST(Medium_ChromeTraceWriter, WritesCountersInstantsAndFlows)
{
    const std::vector<JsonValue> events =
        written_events(::testing::TempDir() + "sushiprofiler_samples.trace.json");

    const JsonValue* const counter = event_named(events, "C", "allocated (B)");
    ASSERT_NE(counter, nullptr);
    EXPECT_EQ(number(*counter->find("args"), "value"), 64.0);
    EXPECT_NE(event_named(events, "i", "underrun"), nullptr);
    const JsonValue* const begin = event_named(events, "s", "item");
    const JsonValue* const end = event_named(events, "f", "item");
    ASSERT_NE(begin, nullptr);
    ASSERT_NE(end, nullptr);
    EXPECT_EQ(number(*begin, "id"), 9.0);
    EXPECT_EQ(number(*end, "id"), 9.0);
    EXPECT_EQ(number(*end, "ts"), 2.9);
}

TEST(Medium_ChromeTraceWriter, WritesSpansOnDeviceTrackRows)
{
    const std::vector<JsonValue> events =
        written_events(::testing::TempDir() + "sushiprofiler_spans.trace.json");

    const JsonValue* const gemm = event_named(events, "X", "gemm");
    ASSERT_NE(gemm, nullptr);
    EXPECT_EQ(number(*gemm, "pid"), 2.0);
    EXPECT_EQ(number(*gemm, "dur"), 1.0);
    EXPECT_EQ(number(*gemm->find("args"), "bytes"), 4096.0);
    EXPECT_EQ(number(*gemm->find("args"), "queued_us"), 1.2);
    EXPECT_EQ(row_name(events, 2.0, 0.0), "queue 0");
}

TEST(Medium_ChromeTraceWriter, AnUnwritablePathIsReported)
{
    const RunRecord record = trace_record();
    ChromeTraceWriter writer(::testing::TempDir() + "missing_folder/none/trace.json");

    EXPECT_EQ(writer.begin(record), WriteStatus::PathUnwritable);
    EXPECT_EQ(writer.write_period(record, trace_period(), {}), WriteStatus::StreamFailed);
    EXPECT_EQ(writer.end(record), WriteStatus::StreamFailed);
}
