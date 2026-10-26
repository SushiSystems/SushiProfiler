/**************************************************************************/
/* test_profiler_config.cpp                                               */
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
 * @file test_profiler_config.cpp
 * @brief Tests the configuration's full form, its defaults and each refusal with its path.
 * @author Mustafa Garip
 * @see tests/session/config/README.md
 */

#include <SushiProfiler/config/profiler_config.hpp>

#include <gtest/gtest.h>

#include <string>

using SushiProfiler::ConfigReadResult;
using SushiProfiler::ProfilerConfig;
using SushiProfiler::read_profiler_config;

namespace
{
    /// @brief Returns the refusal of @p text.
    std::string refusal(const std::string& text)
    {
        const ConfigReadResult result = read_profiler_config(text);
        EXPECT_FALSE(result.ok());
        return result.error;
    }
} // namespace

TEST(Small_ProfilerConfig, ReadsTheDesignedExample)
{
    const ConfigReadResult result = read_profiler_config(R"({
        "sushiprofiler": 1,
        "enabled": true,
        "level": 2,
        "groups": ["Data", "Device", "Host stage"],
        "timeline": "train.step",
        "window": {"skip": 0, "wait": 0, "warmup": 3, "active": 20, "repeat": 1},
        "sources": {"sushiruntime.device": {}, "sushiai.attribution": {}},
        "analyses": ["breakdown", "device_idle", "top_spans", "roofline"],
        "writers": {"terminal": {"top": 20}, "record": {"path": "run.sprec.json"}},
        "output": "build/profiles/yolov5n",
        "buffers": {"ring_events": 65536, "attribute_bytes": 1048576, "linger_ms": 250}
    })");

    ASSERT_TRUE(result.ok()) << result.error;
    const ProfilerConfig& config = *result.config;
    EXPECT_EQ(config.groups, (std::vector<std::string>{"Data", "Device", "Host stage"}));
    EXPECT_EQ(config.timeline, "train.step");
    EXPECT_EQ(config.window.warmup, 3U);
    ASSERT_EQ(config.sources.size(), 2U);
    EXPECT_EQ(config.sources[1].name, "sushiai.attribution");
    EXPECT_EQ(config.analyses.size(), 4U);
    ASSERT_EQ(config.writers.size(), 2U);
    EXPECT_EQ(config.writers[1].name, "record");
    EXPECT_EQ(*config.writers[1].settings.find("path")->as_string(), "run.sprec.json");
    EXPECT_EQ(config.output, "build/profiles/yolov5n");
    EXPECT_EQ(config.buffers.linger_ms, 250U);
}

TEST(Small_ProfilerConfig, AKeyLeftOutKeepsItsDefault)
{
    const ConfigReadResult result = read_profiler_config(R"({"sushiprofiler": 1, "level": 1})");

    ASSERT_TRUE(result.ok()) << result.error;
    const ProfilerConfig& config = *result.config;
    EXPECT_TRUE(config.enabled);
    EXPECT_EQ(config.level, 1U);
    EXPECT_TRUE(config.timeline.empty());
    EXPECT_EQ(config.window.active, 20U);
    EXPECT_EQ(config.window.repeat, 1U);
    EXPECT_EQ(config.buffers.ring_events, 65'536U);
    EXPECT_EQ(config.buffers.linger_ms, 100U);
    EXPECT_TRUE(config.writers.empty());
}

TEST(Small_ProfilerConfig, RefusesAMissingOrOtherVersion)
{
    EXPECT_EQ(refusal(R"({"level": 1})"), "config: missing key \"sushiprofiler\"");
    EXPECT_EQ(refusal(R"({"sushiprofiler": 2})"),
              "sushiprofiler: version 2 is not the version this build reads, 1");
}

TEST(Small_ProfilerConfig, RefusesUnknownKeysWithTheirPath)
{
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "verbose": true})"),
              "config: unknown key \"verbose\"");
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "window": {"size": 3}})"),
              "window: unknown key \"size\"");
}

TEST(Small_ProfilerConfig, RefusesValuesOutOfRange)
{
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "level": 4})"), "level: out of range 0..3");
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "window": {"active": 0}})"),
              "window.active: out of range 1..4294967295");
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "buffers": {"ring_events": 8}})"),
              "buffers.ring_events: out of range 64..16777216");
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "buffers": {"linger_ms": 70000}})"),
              "buffers.linger_ms: out of range 0..60000");
}

TEST(Small_ProfilerConfig, RefusesValuesOfTheWrongKind)
{
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "groups": ["Data", 3]})"),
              "groups[1]: a string was expected");
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "writers": {"terminal": 20}})"),
              "writers.terminal: settings must be an object");
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "enabled": "yes"})"),
              "enabled: a bool was expected, found a string");
}

TEST(Small_ProfilerConfig, ReadsActiveAll)
{
    const ConfigReadResult result =
        read_profiler_config(R"({"sushiprofiler": 1, "window": {"warmup": 2, "active": "all"}})");

    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_TRUE(result.config->window.active_all);
    EXPECT_EQ(result.config->window.warmup, 2U);
}

TEST(Small_ProfilerConfig, AWholeActiveLeavesActiveAllOff)
{
    const ConfigReadResult result =
        read_profiler_config(R"({"sushiprofiler": 1, "window": {"active": 5}})");

    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_FALSE(result.config->window.active_all);
    EXPECT_EQ(result.config->window.active, 5U);
}

TEST(Small_ProfilerConfig, RefusesAnActiveStringOtherThanAll)
{
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "window": {"active": "every"}})"),
              "window.active: a whole number or \"all\" was expected");
}

TEST(Small_ProfilerConfig, RefusesAllWithARepeatOfTwo)
{
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "window": {"active": "all", "repeat": 2}})"),
              "window.repeat: must be 1 when active is \"all\"");
}

TEST(Small_ProfilerConfig, AcceptsAllWithRepeatOne)
{
    const ConfigReadResult result = read_profiler_config(
        R"({"sushiprofiler": 1, "window": {"active": "all", "repeat": 1}})");

    EXPECT_TRUE(result.ok()) << result.error;
}

TEST(Small_ProfilerConfig, ReadsKeepPeriodsIncludingZeroAndLeavesItAbsentByDefault)
{
    const ConfigReadResult none = read_profiler_config(R"({"sushiprofiler": 1})");
    const ConfigReadResult zero =
        read_profiler_config(R"({"sushiprofiler": 1, "record": {"keep_periods": 0}})");
    const ConfigReadResult many =
        read_profiler_config(R"({"sushiprofiler": 1, "record": {"keep_periods": 300}})");

    ASSERT_TRUE(none.ok() && zero.ok() && many.ok());
    EXPECT_FALSE(none.config->record.keep_periods.has_value());
    EXPECT_EQ(zero.config->record.keep_periods, 0U);
    EXPECT_EQ(many.config->record.keep_periods, 300U);
}

TEST(Small_ProfilerConfig, ReadsTheLiveKeysAndKeepsTheirDefaults)
{
    const ConfigReadResult defaults = read_profiler_config(R"({"sushiprofiler": 1})");
    const ConfigReadResult set = read_profiler_config(
        R"({"sushiprofiler": 1, "live": {"history": 60, "max_runs": 0, "min_run_ns": 5,
            "max_poll_ms": 8}})");

    ASSERT_TRUE(defaults.ok() && set.ok()) << set.error;
    EXPECT_EQ(defaults.config->live.history, 300U);
    EXPECT_EQ(defaults.config->live.max_runs, 2048U);
    EXPECT_EQ(defaults.config->live.min_run_ns, 1000U);
    EXPECT_EQ(defaults.config->live.max_poll_ms, 4U);
    EXPECT_EQ(set.config->live.history, 60U);
    EXPECT_EQ(set.config->live.max_runs, 0U);
    EXPECT_EQ(set.config->live.min_run_ns, 5U);
    EXPECT_EQ(set.config->live.max_poll_ms, 8U);
}

TEST(Small_ProfilerConfig, RefusesALiveKeyOutOfRange)
{
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "live": {"max_poll_ms": 0}})"),
              "live.max_poll_ms: out of range 1..1000");
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "live": {"max_poll_ms": 1001}})"),
              "live.max_poll_ms: out of range 1..1000");
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "live": {"history": 0}})"),
              "live.history: out of range 1..65536");
}

TEST(Small_ProfilerConfig, ReadsAWriterEnabledFlagAndHidesItFromTheSettings)
{
    const ConfigReadResult result = read_profiler_config(
        R"({"sushiprofiler": 1, "writers": {"chrome_trace": {"path": "t.json",
            "enabled": false}, "record": {"path": "r.json"}}})");

    ASSERT_TRUE(result.ok()) << result.error;
    ASSERT_EQ(result.config->writers.size(), 2U);
    EXPECT_FALSE(result.config->writers[0].enabled);
    EXPECT_EQ(result.config->writers[0].settings.find("enabled"), nullptr);
    EXPECT_NE(result.config->writers[0].settings.find("path"), nullptr);
    EXPECT_TRUE(result.config->writers[1].enabled);
}

TEST(Small_ProfilerConfig, RefusesAWriterEnabledFlagThatIsNotABool)
{
    EXPECT_FALSE(
        read_profiler_config(R"({"sushiprofiler": 1, "writers": {"a": {"enabled": 1}}})").ok());
}

TEST(Small_ProfilerConfig, LeavesASourcesEnabledKeyInItsSettings)
{
    const ConfigReadResult result = read_profiler_config(
        R"({"sushiprofiler": 1, "sources": {"device": {"enabled": false}}})");

    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_NE(result.config->sources[0].settings.find("enabled"), nullptr);
}

TEST(Small_ProfilerConfig, RefusesAnUnknownRecordKey)
{
    EXPECT_EQ(refusal(R"({"sushiprofiler": 1, "record": {"keep": 3}})"),
              "record: unknown key \"keep\"");
}
