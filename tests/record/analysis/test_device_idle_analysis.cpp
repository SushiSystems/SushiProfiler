/**************************************************************************/
/* test_device_idle_analysis.cpp                                          */
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
 * @file test_device_idle_analysis.cpp
 * @brief Tests device idle time per track and its split into launch and host gaps.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/device_idle_analysis.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <utility>
#include <vector>

#include "analysis_record.hpp"

using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::SiteRecord;
using SushiProfiler::Span;
using SushiProfiler::Table;
using SushiProfiler::ZoneRun;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::span_record;
using SushiProfilerTests::text;
using SushiProfilerTests::whole;

namespace
{
    /// @brief Returns a record of one 600 ns period with one device span from 0 to 100 and one
    /// from 400 to 500 queued at @p queued, submitted by thread 0, over host zones @p runs.
    RunRecord gap_record(std::uint64_t queued, std::vector<ZoneRun> runs)
    {
        RunRecord record;
        record.names = {"", "step"};
        record.sites = {SiteRecord{}, SiteRecord{}, SiteRecord{}};
        record.sites[1].name = "data_wait";
        record.sites[2].name = "decode";
        record.tracks = {"", "queue 0"};
        PeriodRecord period;
        period.timeline = 1U;
        period.end_ns = 600U;
        period.spans.append(Span{0U, 0U, 100U, 1U, 1U, 0U, 0U});
        period.spans.append(Span{queued, 400U, 500U, 1U, 1U, 0U, 0U});
        period.runs = std::move(runs);
        record.periods.push_back(std::move(period));
        return record;
    }

    /// @brief Returns the blamed zone and its share of the one track's idle time.
    std::pair<std::string, double> blame_of(const RunRecord& record)
    {
        const Table table = SushiProfiler::DeviceIdleAnalysis().run(record, {});
        EXPECT_EQ(table.rows.size(), 1U);
        if (table.rows.size() != 1U)
            return {};
        const SushiProfiler::Row& row = table.rows[0];
        return {*row[table.column("blamed_zone")].as_text(),
                *row[table.column("blamed_share")].as_real()};
    }
} // namespace

TEST(Small_DeviceIdleAnalysis, SplitsEachTracksIdleTimeByCause)
{
    const Table table = SushiProfiler::DeviceIdleAnalysis().run(span_record(), {});

    ASSERT_EQ(table.rows.size(), 1U);
    expect_row(table.rows[0], {text("queue 0"), whole(2), real(393.5), real(392.0), real(395.0),
                               real(392.0), real(394.85), real(std::sqrt(4.5) / 393.5),
                               real(787.0 / 2000.0), real(200.0 / 787.0), real(587.0 / 787.0),
                               text("unattributed"), real(587.0 / 787.0)});
}

TEST(Small_DeviceIdleAnalysis, NamesTheInnermostHostZoneOpenOnTheSubmittingThreadAtTheGapsStart)
{
    const RunRecord record = gap_record(300U, {ZoneRun{0U, 0U, 1U, 0U, 50U, 600U, 0U},
                                               ZoneRun{0U, 0U, 2U, 0U, 90U, 450U, 1U}});

    const auto blame = blame_of(record);

    EXPECT_EQ(blame.first, "decode");
    EXPECT_NEAR(blame.second, 300.0 / 400.0, 1e-12);
}

TEST(Small_DeviceIdleAnalysis, IgnoresAZoneOpenOnAnotherThread)
{
    const RunRecord record = gap_record(300U, {ZoneRun{1U, 0U, 1U, 0U, 0U, 600U, 0U}});

    const auto blame = blame_of(record);

    EXPECT_EQ(blame.first, "unattributed");
    EXPECT_NEAR(blame.second, 1.0, 1e-12);
}

TEST(Small_DeviceIdleAnalysis, ChargesAGapToLaunchWhenNoZoneIsOpenAndTheNextSpanWasQueued)
{
    const RunRecord record = gap_record(50U, {});

    const auto blame = blame_of(record);

    EXPECT_EQ(blame.first, "launch");
    EXPECT_NEAR(blame.second, 300.0 / 400.0, 1e-12);
}

TEST(Small_DeviceIdleAnalysis, ARecordWithoutSpansHasNoRows)
{
    const Table table =
        SushiProfiler::DeviceIdleAnalysis().run(SushiProfilerTests::analysis_record(), {});

    EXPECT_TRUE(table.rows.empty());
}
