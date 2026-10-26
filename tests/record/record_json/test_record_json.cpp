/**************************************************************************/
/* test_record_json.cpp                                                   */
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
 * @file test_record_json.cpp
 * @brief Tests the record format: byte-for-byte round trips and each refusal with its path.
 * @author Mustafa Garip
 * @see tests/record/record_json/README.md
 */

#include <SushiProfiler/record_json/record_json.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <string>

using SushiProfiler::AttributeKind;
using SushiProfiler::CounterOp;
using SushiProfiler::FlowPoint;
using SushiProfiler::PeriodRecord;
using SushiProfiler::read_record;
using SushiProfiler::RecordReadResult;
using SushiProfiler::RunRecord;
using SushiProfiler::SampleSeries;
using SushiProfiler::SiteRecord;
using SushiProfiler::SiteRole;
using SushiProfiler::Span;
using SushiProfiler::ThreadRecord;
using SushiProfiler::write_record;
using SushiProfiler::ZoneNode;
using SushiProfiler::ZoneRun;
using SushiProfiler::ZoneTree;

namespace
{
    /// @brief Returns a site with @p name, @p role and @p op.
    SiteRecord site(const char* name, SiteRole role, CounterOp op = CounterOp::Add)
    {
        SiteRecord site;
        site.name = name;
        site.group = "Host";
        site.file = "train.cpp";
        site.line = 40U;
        site.role = role;
        site.op = op;
        return site;
    }

    /// @brief Returns a record that uses every table, column and kind of the format.
    RunRecord full_record()
    {
        RunRecord record;
        record.environment = {{"os", "Windows"}, {"cpu", "Ryzen 5 7600X"}};
        record.build = {{"type", "Release"}};
        record.tags = {{"run", "baseline"}};
        record.clock = {"tsc", 0.2, 6.5};
        record.names = {"", "train.step", "lane.compute", "kernel", "batch", "size", "ratio",
                        "device"};
        record.sites = {SiteRecord{}, site("forward", SiteRole::Zone),
                        site("backward", SiteRole::Zone),
                        site("bytes", SiteRole::Counter, CounterOp::Max),
                        site("item", SiteRole::Flow)};
        record.sites[3].unit = "B";
        record.tracks = {"", "queue 0"};
        record.threads = {ThreadRecord{7U, "main", 2U, 1U, 5U},
                          ThreadRecord{3U, "worker", 0U, 0U, 0U}};
        record.drops = {1U, 2U, 3U};

        PeriodRecord busy;
        busy.timeline = 1U;
        busy.thread = 7U;
        busy.begin_ns = 1'000U;
        busy.end_ns = 5'000'001'000U;
        busy.warmup = true;
        busy.tags = {{4U, 5U}};
        busy.facts = {{"layer", "fused \"conv\""}};
        ZoneTree tree;
        tree.thread = 7U;
        tree.lane = 2U;
        tree.nodes = {ZoneNode{1U, 0U, -1, 400U, 300U, 1U}, ZoneNode{2U, 3U, 0, 100U, 100U, 2U}};
        busy.zones.push_back(tree);
        busy.runs = {ZoneRun{7U, 2U, 1U, 0U, 1'000U, 1'400U, 0U},
                     ZoneRun{7U, 2U, 1U, 3U, 1'100U, 1'200U, 1U}};
        const std::size_t first = busy.spans.append(Span{90U, 100U, 150U, 1U, 3U, 3U, 0U});
        const std::size_t second = busy.spans.append(Span{95U, 200U, 260U, 1U, 3U, 3U, 12U});
        busy.spans.set_integer(first, 5U, -12);
        busy.spans.set_real(second, 6U, std::numeric_limits<double>::infinity());
        busy.spans.set_name(first, 7U, 3U);
        SampleSeries counter;
        counter.site = 3U;
        counter.thread = 3U;
        counter.time_ns = {10U, 20U};
        counter.values = {1.5, -0.0};
        SampleSeries flow;
        flow.site = 4U;
        flow.thread = 7U;
        flow.time_ns = {30U, 40U};
        flow.items = {9U, 9U};
        flow.points = {FlowPoint::Begin, FlowPoint::End};
        busy.samples = {counter, flow};
        record.periods.push_back(busy);

        PeriodRecord idle;
        idle.index = 1U;
        idle.timeline = 1U;
        idle.thread = 7U;
        idle.begin_ns = 5'000'001'000U;
        idle.end_ns = 5'000'002'000U;
        record.periods.push_back(idle);
        return record;
    }

    /// @brief Returns @p text with its one occurrence of @p from replaced by @p to.
    std::string replaced(const std::string& text, const std::string& from, const std::string& to)
    {
        const std::size_t at = text.find(from);
        EXPECT_NE(at, std::string::npos) << from;
        EXPECT_EQ(text.find(from, at + 1U), std::string::npos) << from;
        if (at == std::string::npos)
            return text;
        return text.substr(0U, at) + to + text.substr(at + from.size());
    }

    /// @brief Returns the refusal of the full record's text after one replacement.
    std::string refusal(const std::string& from, const std::string& to)
    {
        const std::string text = replaced(write_record(full_record()), from, to);
        const RecordReadResult result = read_record(text);
        EXPECT_FALSE(result.ok());
        return result.error;
    }
} // namespace

TEST(Small_RecordJson, AFullRecordRoundTripsByteForByte)
{
    const RunRecord record = full_record();
    const std::string text = write_record(record);
    const RecordReadResult result = read_record(text);

    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_EQ(*result.record, record);
    EXPECT_EQ(write_record(*result.record), text);
}

TEST(Small_RecordJson, AnEmptyRecordRoundTrips)
{
    const RunRecord record;
    const RecordReadResult result = read_record(write_record(record));

    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_EQ(*result.record, record);
}

TEST(Small_RecordJson, ColumnsSitOnOneLine)
{
    const std::string text = write_record(full_record());

    EXPECT_NE(text.find("\"parent\": [-1, 0]"), std::string::npos);
    EXPECT_NE(text.find("\"points\": [\"begin\", \"end\"]"), std::string::npos);
    EXPECT_EQ(text.rfind("{\n  \"sushiprofiler_record\": 2,\n", 0U), 0U);
}

TEST(Small_RecordJson, NonFiniteRealsAreWrittenAsWords)
{
    RunRecord record;
    record.clock.read_cost_ns = std::nan("");
    record.clock.nanoseconds_per_tick = -std::numeric_limits<double>::infinity();
    const std::string text = write_record(record);
    const RecordReadResult result = read_record(text);

    EXPECT_NE(text.find("\"read_cost_ns\": \"NaN\""), std::string::npos);
    EXPECT_NE(text.find("\"nanoseconds_per_tick\": \"-Infinity\""), std::string::npos);
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_TRUE(std::isnan(result.record->clock.read_cost_ns));
    EXPECT_EQ(result.record->clock.nanoseconds_per_tick, -std::numeric_limits<double>::infinity());
}

TEST(Small_RecordJson, ANewerVersionIsRefused)
{
    EXPECT_EQ(refusal("\"sushiprofiler_record\": 2,", "\"sushiprofiler_record\": 3,"),
              "sushiprofiler_record: version 3 is not the version this build reads, 2");
}

TEST(Small_RecordJson, TheVersionBeforeZoneRunsIsRefusedByItsNumber)
{
    EXPECT_EQ(refusal("\"sushiprofiler_record\": 2,", "\"sushiprofiler_record\": 1,"),
              "sushiprofiler_record: version 1 is not the version this build reads, 2");
}

TEST(Small_RecordJson, AnUnknownKeyIsRefusedWithItsPath)
{
    EXPECT_EQ(refusal("\"read_cost_ns\": 6.5}", "\"read_cost_ns\": 6.5, \"extra\": 1}"),
              "clock: unknown key \"extra\"");
}

TEST(Small_RecordJson, AMissingKeyIsRefusedWithItsPath)
{
    EXPECT_EQ(refusal(", \"spans_dropped\": 3}", "}"), "drops: missing key \"spans_dropped\"");
}

TEST(Small_RecordJson, AValueOutOfRangeIsRefusedWithItsPath)
{
    EXPECT_EQ(refusal("\"index\": 7, \"name\": \"main\"",
                      "\"index\": 4294967296, \"name\": \"main\""),
              "threads[0].index: out of range 0..4294967295");
}

TEST(Small_RecordJson, AValueOfTheWrongKindIsRefusedWithItsPath)
{
    EXPECT_EQ(refusal("\"warmup\": true", "\"warmup\": 1"),
              "periods[0].warmup: a bool was expected, found a whole number");
}

TEST(Small_RecordJson, ColumnsOfUnequalLengthAreRefused)
{
    EXPECT_EQ(refusal("\"start_ns\": [100, 200]", "\"start_ns\": [100]"),
              "periods[0].spans.start_ns: has 1 entries where 2 were expected");
}

TEST(Small_RecordJson, ASiteThatPointsNowhereIsRefused)
{
    EXPECT_EQ(refusal("\"site\": [1, 2]", "\"site\": [1, 99]"),
              "periods[0].zones[0].site[1]: points past the sites table");
}

TEST(Small_RecordJson, AZoneRunOnASiteThatPointsNowhereIsRefused)
{
    EXPECT_EQ(refusal("\"site\": [1, 1]", "\"site\": [1, 99]"),
              "periods[0].runs.site[1]: points past the sites table");
}

TEST(Small_RecordJson, AZoneRunThatEndsBeforeItBeginsIsRefused)
{
    EXPECT_EQ(refusal("\"end_ns\": [1400, 1200]", "\"end_ns\": [1400, 1050]"),
              "periods[0].runs.end_ns[1]: ends before it begins");
}

TEST(Small_RecordJson, AZoneRunOnAThreadOutsideTheTableIsRefused)
{
    EXPECT_EQ(refusal("\"thread\": [7, 7]", "\"thread\": [7, 9]"),
              "periods[0].runs.thread: thread 9 is not in the thread table");
}

TEST(Small_RecordJson, AParentThatIsNotEarlierIsRefused)
{
    EXPECT_EQ(refusal("\"parent\": [-1, 0]", "\"parent\": [0, 0]"),
              "periods[0].zones[0].parent[0]: points at no earlier node");
}

TEST(Small_RecordJson, AThreadOutsideTheTableIsRefused)
{
    EXPECT_EQ(refusal("\"thread\": 7,\n      \"begin_ns\": 1000",
                      "\"thread\": 9,\n      \"begin_ns\": 1000"),
              "periods[0].thread: thread 9 is not in the thread table");
}

TEST(Small_RecordJson, AnUnknownWordIsRefused)
{
    EXPECT_EQ(refusal("\"op\": \"max\"", "\"op\": \"mean\""),
              "sites[3].op: \"mean\" is not a known word");
}

TEST(Small_RecordJson, TextThatIsNotJsonNamesItsPlace)
{
    const RecordReadResult result = read_record("{");

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error.rfind("line 1, column 2: ", 0U), 0U) << result.error;
}
