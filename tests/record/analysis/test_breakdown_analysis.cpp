/**************************************************************************/
/* test_breakdown_analysis.cpp                                            */
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
 * @file test_breakdown_analysis.cpp
 * @brief Tests the breakdown rows: self time per group, then the uncovered time.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/breakdown_analysis.hpp>

#include <gtest/gtest.h>

#include <cmath>

#include "analysis_record.hpp"

using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::SiteRecord;
using SushiProfiler::Table;
using SushiProfiler::ZoneNode;
using SushiProfiler::ZoneTree;
using SushiProfilerTests::analysis_record;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::text;
using SushiProfilerTests::whole;

namespace
{
    /// @brief Returns an active 100 ns period of @p timeline whose one zone of group `G` takes
    /// @p zone_ns.
    PeriodRecord period_of(SushiProfiler::NameID timeline, std::uint64_t zone_ns)
    {
        PeriodRecord period;
        period.timeline = timeline;
        period.end_ns = 100U;
        ZoneTree tree;
        tree.nodes = {ZoneNode{1U, 0U, -1, zone_ns, zone_ns, 1U}};
        period.zones = {tree};
        return period;
    }

    /// @brief Returns a record of timelines `alpha` and `beta`, `beta` listed first; their zone
    /// takes 40 and 60 ns of 100.
    RunRecord two_timeline_record()
    {
        RunRecord record;
        record.names = {"", "alpha", "beta"};
        record.sites = {SiteRecord{}, SiteRecord{}};
        record.sites[1].group = "G";
        record.periods = {period_of(2U, 60U), period_of(1U, 40U)};
        return record;
    }
} // namespace

TEST(Small_BreakdownAnalysis, ReportsSelfTimePerGroupThenTheUncoveredTime)
{
    const Table table = SushiProfiler::BreakdownAnalysis().run(analysis_record(), {});

    ASSERT_EQ(table.rows.size(), 3U);
    expect_row(table.rows[0], {text("step"), text("Model"), whole(3), real(70.0), real(60.0),
                               real(80.0), real(60.0), real(79.0), real(10.0 / 70.0),
                               real(210.0 / 330.0)});
    expect_row(table.rows[1], {text("step"), text("Data"), whole(3), real(30.0), real(20.0),
                               real(30.0), real(20.0), real(30.0),
                               real(std::sqrt(100.0 / 3.0) / (80.0 / 3.0)), real(80.0 / 330.0)});
    expect_row(table.rows[2], {text("step"), text("(uncovered)"), whole(3), real(10.0),
                               real(10.0), real(20.0), real(10.0), real(19.0),
                               real(std::sqrt(100.0 / 3.0) / (40.0 / 3.0)), real(40.0 / 330.0)});
}

TEST(Small_BreakdownAnalysis, ReportsEachTimelineWithItsOwnGroupsAndUncoveredRow)
{
    const Table table = SushiProfiler::BreakdownAnalysis().run(two_timeline_record(), {});

    ASSERT_EQ(table.rows.size(), 4U);
    expect_row(table.rows[0], {text("alpha"), text("G"), whole(1), real(40.0), real(40.0),
                               real(40.0), real(40.0), real(40.0), real(0.0), real(0.4)});
    expect_row(table.rows[1], {text("alpha"), text("(uncovered)"), whole(1), real(60.0),
                               real(60.0), real(60.0), real(60.0), real(60.0), real(0.0),
                               real(0.6)});
    expect_row(table.rows[2], {text("beta"), text("G"), whole(1), real(60.0), real(60.0),
                               real(60.0), real(60.0), real(60.0), real(0.0), real(0.6)});
    expect_row(table.rows[3], {text("beta"), text("(uncovered)"), whole(1), real(40.0),
                               real(40.0), real(40.0), real(40.0), real(40.0), real(0.0),
                               real(0.4)});
}
