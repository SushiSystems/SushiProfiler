/**************************************************************************/
/* test_distribution_analysis.cpp                                         */
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
 * @file test_distribution_analysis.cpp
 * @brief Tests the distribution rows of each timeline, zone, counter and value.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/distribution_analysis.hpp>

#include <gtest/gtest.h>

#include "analysis_record.hpp"

using SushiProfiler::Table;
using SushiProfilerTests::analysis_record;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::text;
using SushiProfilerTests::whole;

TEST(Small_DistributionAnalysis, ReportsEachTimelineZoneCounterAndValue)
{
    const Table table = SushiProfiler::DistributionAnalysis().run(analysis_record(), {});

    ASSERT_EQ(table.rows.size(), 7U);
    expect_row(table.rows[0], {text("timeline:step"), text("ns"), whole(3), real(110.0),
                               real(118.0), real(119.8), real(120.0), real(15.0), text("2^6:3")});
    expect_row(table.rows[1], {text("zone:forward"), text("ns"), whole(3), real(70.0), real(78.0),
                               real(79.8), real(80.0), real(15.0), text("2^5:1 2^6:2")});
    expect_row(table.rows[2], {text("zone:conv"), text("ns"), whole(2), real(25.0), real(29.0),
                               real(29.9), real(30.0), real(10.0), text("2^4:2")});
    expect_row(table.rows[3], {text("zone:load"), text("ns"), whole(3), real(30.0), real(30.0),
                               real(30.0), real(30.0), real(10.0), text("2^4:3")});
    expect_row(table.rows[4], {text("counter:bytes"), text("B"), whole(2), real(175.0),
                               real(195.0), real(199.5), real(200.0), real(50.0), text("2^7:2")});
    expect_row(table.rows[5], {text("counter:peak"), text("B"), whole(3), real(7.0), real(8.6),
                               real(8.96), real(9.0), real(4.0), text("2^2:2 2^3:1")});
    expect_row(table.rows[6], {text("value:loss"), text(""), whole(3), real(2.0), real(2.8),
                               real(2.98), real(3.0), real(1.5), text("2^0:1 2^1:2")});
}
