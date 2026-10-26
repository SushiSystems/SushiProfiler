/**************************************************************************/
/* test_roofline_analysis.cpp                                             */
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
 * @file test_roofline_analysis.cpp
 * @brief Tests achieved rates, arithmetic intensity, the attainable bound and the limiting roof.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/roofline_analysis.hpp>

#include <gtest/gtest.h>

#include <string>

#include "analysis_record.hpp"

using SushiProfiler::Cell;
using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::Span;
using SushiProfiler::Table;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::text;
using SushiProfilerTests::whole;

namespace
{
    /// @brief Appends a span of @p name on @p track from 0 to @p length ns with its cost.
    void add_span(PeriodRecord& period, SushiProfiler::NameID name, SushiProfiler::TrackID track,
                  std::uint64_t length, std::int64_t flops, std::int64_t bytes)
    {
        const std::size_t row = period.spans.append(Span{0U, 0U, length, track, name, 0U, 0U});
        period.spans.set_integer(row, 3U, flops);
        period.spans.set_integer(row, 4U, bytes);
    }

    /// @brief Returns a record whose device `queue 0` has ceilings of 1e12 FLOP/s and 1e11 B/s.
    ///
    /// Period 1 holds two `gemm` spans of 100 us with 5e7 FLOP and 2e6 B each, one `copy` span of
    /// 100 us with 1e6 FLOP and 8e6 B, and two spans without both attributes. Period 2 holds one
    /// `gemm` span on `queue 1`, which has no ceilings.
    RunRecord roofline_record()
    {
        RunRecord record;
        record.names = {"", "gemm", "copy", "flops", "bytes", "other"};
        record.tracks = {"", "queue 0", "queue 1"};
        record.environment = {{"device.queue 0.peak_flops", "1e12"},
                              {"device.queue 0.peak_bytes_per_s", "100000000000"}};
        PeriodRecord first;
        first.index = 1U;
        first.timeline = 1U;
        first.end_ns = 1'000'000U;
        add_span(first, 1U, 1U, 100'000U, 50'000'000, 2'000'000);
        add_span(first, 1U, 1U, 100'000U, 50'000'000, 2'000'000);
        add_span(first, 2U, 1U, 100'000U, 1'000'000, 8'000'000);
        first.spans.append(Span{0U, 0U, 100U, 1U, 5U, 0U, 0U});
        const std::size_t half = first.spans.append(Span{0U, 0U, 100U, 1U, 1U, 0U, 0U});
        first.spans.set_integer(half, 3U, 7);
        PeriodRecord second;
        second.index = 2U;
        second.timeline = 1U;
        second.begin_ns = 1'000'000U;
        second.end_ns = 2'000'000U;
        add_span(second, 1U, 2U, 200'000U, 20'000'000, 1'000'000);
        PeriodRecord warmup;
        warmup.index = 0U;
        warmup.timeline = 1U;
        warmup.warmup = true;
        add_span(warmup, 1U, 1U, 1'000U, 1, 1);
        record.periods = {warmup, first, second};
        return record;
    }
} // namespace

namespace
{
    /// @brief Appends a `gemm` span of 100 us on `queue 0` with its cost, @p node when not
    /// negative and @p dtype when not 0.
    void add_typed(PeriodRecord& period, std::int64_t node, SushiProfiler::NameID dtype,
                   std::int64_t flops, std::int64_t bytes)
    {
        const std::size_t row = period.spans.append(Span{0U, 0U, 100'000U, 1U, 1U, 0U, 0U});
        period.spans.set_integer(row, 2U, flops);
        period.spans.set_integer(row, 3U, bytes);
        if (node >= 0)
            period.spans.set_integer(row, 4U, node);
        if (dtype != 0U)
            period.spans.set_name(row, 5U, dtype);
    }

    /// @brief Returns a record of one period on `queue 0`, whose ceilings are 1e11 B/s, 1e12 FLOP/s
    /// and, for `f16`, 4e12 FLOP/s.
    ///
    /// The spans, each of 100 us: two on node 7 in `f16` with 1e8 FLOP and 1e6 B each; one on node
    /// 8 in `f16` with 5e7 FLOP and 1e6 B; one on node 8 in `f32` with 1e8 FLOP and 1e6 B; one
    /// with no node and no type, with no FLOP and 8e6 B.
    RunRecord typed_record()
    {
        RunRecord record;
        record.names = {"", "gemm", "flops", "bytes", "node", "dtype", "f16", "f32"};
        record.tracks = {"", "queue 0"};
        record.environment = {{"device.queue 0.peak_flops", "1e12"},
                              {"device.queue 0.peak_flops.f16", "4e12"},
                              {"device.queue 0.peak_bytes_per_s", "1e11"}};
        PeriodRecord period;
        period.index = 1U;
        period.timeline = 1U;
        period.end_ns = 1'000'000U;
        add_typed(period, 7, 6U, 100'000'000, 1'000'000);
        add_typed(period, 7, 6U, 100'000'000, 1'000'000);
        add_typed(period, 8, 6U, 50'000'000, 1'000'000);
        add_typed(period, 8, 7U, 100'000'000, 1'000'000);
        add_typed(period, -1, 0U, 0, 8'000'000);
        record.periods = {period};
        return record;
    }
} // namespace

TEST(Small_RooflineAnalysis, GivesEachNodeItsRowAndSplitsAMixedTypeAndMergesTheSameOne)
{
    const Table table = SushiProfiler::RooflineAnalysis().run(typed_record(), {});

    ASSERT_EQ(table.rows.size(), 4U);
    EXPECT_TRUE(table.rows[0][5].empty());
    EXPECT_EQ(*table.rows[1][5].as_whole(), 7);
    EXPECT_EQ(*table.rows[1][6].as_text(), "f16");
    EXPECT_EQ(*table.rows[1][7].as_whole(), 2);
    EXPECT_EQ(*table.rows[2][5].as_whole(), 8);
    EXPECT_EQ(*table.rows[2][6].as_text(), "f16");
    EXPECT_EQ(*table.rows[3][5].as_whole(), 8);
    EXPECT_EQ(*table.rows[3][6].as_text(), "f32");
}

TEST(Small_RooflineAnalysis, PicksTheCeilingOfTheSpansTypeAndFallsBackToTheTypelessOne)
{
    const Table table = SushiProfiler::RooflineAnalysis().run(typed_record(), {});

    ASSERT_EQ(table.rows.size(), 4U);
    expect_row(table.rows[1],
               {whole(1), Cell(), Cell(), text("gemm"), text("queue 0"), whole(7), text("f16"), whole(2),
                whole(200'000), whole(200'000'000), whole(2'000'000), real(1e12), real(1e10),
                real(100.0), real(4e12), real(1e11), real(4e12), real(0.25), text("compute")});
    expect_row(table.rows[2],
               {whole(1), Cell(), Cell(), text("gemm"), text("queue 0"), whole(8), text("f16"), whole(1),
                whole(100'000), whole(50'000'000), whole(1'000'000), real(5e11), real(1e10),
                real(50.0), real(4e12), real(1e11), real(4e12), real(0.125), text("compute")});
    expect_row(table.rows[3],
               {whole(1), Cell(), Cell(), text("gemm"), text("queue 0"), whole(8), text("f32"), whole(1),
                whole(100'000), whole(100'000'000), whole(1'000'000), real(1e12), real(1e10),
                real(100.0), real(1e12), real(1e11), real(1e12), real(1.0), text("compute")});
}

TEST(Small_RooflineAnalysis, ARowWithNoFlopsButBytesIsMeasuredAgainstTheMemoryCeiling)
{
    const Table table = SushiProfiler::RooflineAnalysis().run(typed_record(), {});

    ASSERT_EQ(table.rows.size(), 4U);
    expect_row(table.rows[0],
               {whole(1), Cell(), Cell(), text("gemm"), text("queue 0"), Cell(), Cell(), whole(1),
                whole(100'000), whole(0), whole(8'000'000), real(0.0), real(8e10), real(0.0),
                real(1e12), real(1e11), real(0.0), real(0.8), text("memory")});
}

TEST(Small_RooflineAnalysis, ReportsAchievedRatesAndIntensityPerPeriodNameAndTrack)
{
    const Table table = SushiProfiler::RooflineAnalysis().run(roofline_record(), {});

    ASSERT_EQ(table.rows.size(), 3U);
    expect_row(table.rows[0],
               {whole(1), Cell(), Cell(), text("copy"), text("queue 0"), Cell(), Cell(), whole(1), whole(100'000),
                whole(1'000'000), whole(8'000'000), real(1e10), real(8e10), real(0.125),
                real(1e12), real(1e11), real(1.25e10), real(0.8), text("memory")});
    expect_row(table.rows[1],
               {whole(1), Cell(), Cell(), text("gemm"), text("queue 0"), Cell(), Cell(), whole(2), whole(200'000),
                whole(100'000'000), whole(4'000'000), real(5e11), real(2e10), real(25.0),
                real(1e12), real(1e11), real(1e12), real(0.5), text("compute")});
}

TEST(Small_RooflineAnalysis, LeavesTheCeilingCellsEmptyWhenTheTrackHasNoFacts)
{
    const Table table = SushiProfiler::RooflineAnalysis().run(roofline_record(), {});

    ASSERT_EQ(table.rows.size(), 3U);
    const SushiProfiler::Row& row = table.rows[2];
    EXPECT_EQ(*row[0].as_whole(), 2);
    EXPECT_EQ(*row[4].as_text(), "queue 1");
    EXPECT_NEAR(*row[11].as_real(), 1e11, 1.0);
    EXPECT_NEAR(*row[13].as_real(), 20.0, 1e-9);
    for (std::size_t column = 14U; column < row.size(); ++column)
        EXPECT_TRUE(row[column].empty()) << "column " << column;
}

TEST(Small_RooflineAnalysis, CountsSpansWithoutBothAttributesAndSkipsWarmupPeriods)
{
    const Table table = SushiProfiler::RooflineAnalysis().run(roofline_record(), {});

    ASSERT_EQ(table.notes.size(), 1U);
    EXPECT_EQ(table.notes[0], "2 spans without both flops and bytes were skipped.");
    for (const SushiProfiler::Row& row : table.rows)
        EXPECT_NE(*row[0].as_whole(), 0);
}

TEST(Small_RooflineAnalysis, ARecordWithoutCostAttributesHasNoRowsAndCountsEverySpan)
{
    const Table table =
        SushiProfiler::RooflineAnalysis().run(SushiProfilerTests::span_record(), {});

    EXPECT_TRUE(table.rows.empty());
    ASSERT_EQ(table.notes.size(), 1U);
    EXPECT_EQ(table.notes[0], "5 spans without both flops and bytes were skipped.");
}

TEST(Small_RooflineAnalysis, AnUnreadableCeilingFactIsTreatedAsMissing)
{
    RunRecord record = roofline_record();
    record.environment[1].value = "fast";

    const Table table = SushiProfiler::RooflineAnalysis().run(record, {});

    ASSERT_EQ(table.rows.size(), 3U);
    EXPECT_TRUE(table.rows[0][18].empty());
}

namespace
{
    /// @brief Appends a `gemm` span of 100 us on `queue 0` with 1e8 FLOP and 1e6 B, on node 3,
    /// with @p graph and @p origin when not 0.
    void add_labelled(PeriodRecord& period, SushiProfiler::NameID graph,
                      SushiProfiler::NameID origin)
    {
        const std::size_t row = period.spans.append(Span{0U, 0U, 100'000U, 1U, 1U, 0U, 0U});
        period.spans.set_integer(row, 2U, 100'000'000);
        period.spans.set_integer(row, 3U, 1'000'000);
        period.spans.set_integer(row, 4U, 3);
        if (graph != 0U)
            period.spans.set_name(row, 5U, graph);
        if (origin != 0U)
            period.spans.set_name(row, 6U, origin);
    }

    /// @brief Returns a record of one period on `queue 0`, without ceilings, whose spans are all
    /// `gemm` on node 3 of 100 us with 1e8 FLOP and 1e6 B.
    ///
    /// The graph and origin of the spans, in order: (`g1`, none), (`g2`, none), (`g1`, `o1`),
    /// (`g1`, `o2`), (`g1`, `o1`), and (none, none).
    RunRecord labelled_record()
    {
        RunRecord record;
        record.names = {"", "gemm", "flops", "bytes", "node", "graph", "origin", "g1", "g2", "o1",
                        "o2"};
        record.tracks = {"", "queue 0"};
        PeriodRecord period;
        period.index = 1U;
        period.timeline = 1U;
        period.end_ns = 1'000'000U;
        add_labelled(period, 7U, 0U);
        add_labelled(period, 8U, 0U);
        add_labelled(period, 7U, 9U);
        add_labelled(period, 7U, 10U);
        add_labelled(period, 7U, 9U);
        add_labelled(period, 0U, 0U);
        record.periods = {period};
        return record;
    }
} // namespace

TEST(Small_RooflineAnalysis, TwoGraphsWithTheSameNodeIndexGiveTwoRows)
{
    const Table table = SushiProfiler::RooflineAnalysis().run(labelled_record(), {});

    ASSERT_EQ(table.rows.size(), 5U);
    EXPECT_TRUE(table.rows[0][1].empty());
    EXPECT_TRUE(table.rows[0][2].empty());
    EXPECT_EQ(*table.rows[1][1].as_text(), "g1");
    EXPECT_TRUE(table.rows[1][2].empty());
    EXPECT_EQ(*table.rows[1][7].as_whole(), 1);
    EXPECT_EQ(*table.rows[4][1].as_text(), "g2");
    EXPECT_EQ(*table.rows[4][7].as_whole(), 1);
}

TEST(Small_RooflineAnalysis, TwoOriginsOfOneGraphGiveTwoRowsAndTheSameOriginMerges)
{
    const Table table = SushiProfiler::RooflineAnalysis().run(labelled_record(), {});

    ASSERT_EQ(table.rows.size(), 5U);
    expect_row(table.rows[2],
               {whole(1), text("g1"), text("o1"), text("gemm"), text("queue 0"), whole(3), Cell(),
                whole(2), whole(200'000), whole(200'000'000), whole(2'000'000), real(1e12),
                real(1e10), real(100.0), Cell(), Cell(), Cell(), Cell(), Cell()});
    expect_row(table.rows[3],
               {whole(1), text("g1"), text("o2"), text("gemm"), text("queue 0"), whole(3), Cell(),
                whole(1), whole(100'000), whole(100'000'000), whole(1'000'000), real(1e12),
                real(1e10), real(100.0), Cell(), Cell(), Cell(), Cell(), Cell()});
}

TEST(Small_RooflineAnalysis, SpansWithoutGraphOrOriginKeepTheirRowAndValues)
{
    const Table table = SushiProfiler::RooflineAnalysis().run(labelled_record(), {});

    ASSERT_EQ(table.rows.size(), 5U);
    expect_row(table.rows[0],
               {whole(1), Cell(), Cell(), text("gemm"), text("queue 0"), whole(3), Cell(),
                whole(1), whole(100'000), whole(100'000'000), whole(1'000'000), real(1e12),
                real(1e10), real(100.0), Cell(), Cell(), Cell(), Cell(), Cell()});
}
