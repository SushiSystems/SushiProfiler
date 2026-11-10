/**************************************************************************/
/* chrome_trace_fixture.hpp                                               */
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
 * @file chrome_trace_fixture.hpp
 * @brief Declares the run, the period and the readers of trace events the Chrome trace tests share.
 * @author Mustafa Garip
 * @see tests/session/stream_writers/README.md
 */

#pragma once

#include <SushiProfiler/json/json_value.hpp>
#include <SushiProfiler/record/period_record.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/sites/site.hpp>
#include <SushiProfiler/stream_writers/stream_writer.hpp>

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace SushiProfilerTests
{
    using SushiProfiler::FlowPoint;
    using SushiProfiler::JsonValue;
    using SushiProfiler::PeriodRecord;
    using SushiProfiler::RunRecord;
    using SushiProfiler::SampleSeries;
    using SushiProfiler::SiteRecord;
    using SushiProfiler::SiteRole;
    using SushiProfiler::Span;
    using SushiProfiler::ThreadRecord;

    /// @brief Returns a site of @p role named @p name with @p group and @p unit.
    inline SiteRecord site_of(SiteRole role, std::string name, std::string group = "",
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
    inline RunRecord trace_record()
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
    inline PeriodRecord trace_period()
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

    /// @brief Returns two zone runs: `conv` on thread 0 and `conv 3x3` on its lane `voice`.
    inline std::vector<SushiProfiler::ZoneRun> trace_instances()
    {
        return {SushiProfiler::ZoneRun{0U, 0U, 1U, 0U, 1'000U, 2'500U, 0U},
                SushiProfiler::ZoneRun{0U, 2U, 1U, 5U, 1'200U, 1'300U, 0U}};
    }

    /// @brief Returns the first event of phase @p phase named @p name, or null.
    inline const JsonValue* event_named(const std::vector<JsonValue>& events,
                                        const std::string& phase, const std::string& name)
    {
        for (const JsonValue& event : events)
        {
            if (*event.find("ph")->as_string() == phase && *event.find("name")->as_string() == name)
                return &event;
        }
        return nullptr;
    }

    /// @brief Returns member @p key of @p event as a number.
    inline double number(const JsonValue& event, const char* key)
    {
        double value = 0.0;
        EXPECT_TRUE(event.find(key)->to_double(value)) << key;
        return value;
    }

    /// @brief Returns the row name a metadata event gives @p pid and @p tid, or an empty string.
    inline std::string row_name(const std::vector<JsonValue>& events, double pid, double tid)
    {
        for (const JsonValue& event : events)
        {
            if (*event.find("name")->as_string() == "thread_name" &&
                number(event, "pid") == pid && number(event, "tid") == tid)
                return *event.find("args")->find("name")->as_string();
        }
        return std::string();
    }
} // namespace SushiProfilerTests
