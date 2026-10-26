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

#include "analysis_record.hpp"

using SushiProfiler::Table;
using SushiProfilerTests::expect_row;
using SushiProfilerTests::real;
using SushiProfilerTests::span_record;
using SushiProfilerTests::text;
using SushiProfilerTests::whole;

TEST(Small_DeviceIdleAnalysis, SplitsEachTracksIdleTimeByCause)
{
    const Table table = SushiProfiler::DeviceIdleAnalysis().run(span_record(), {});

    ASSERT_EQ(table.rows.size(), 1U);
    expect_row(table.rows[0], {text("queue 0"), whole(2), real(393.5), real(392.0), real(395.0),
                               real(392.0), real(394.85), real(std::sqrt(4.5) / 393.5),
                               real(787.0 / 2000.0), real(200.0 / 787.0), real(587.0 / 787.0)});
}

TEST(Small_DeviceIdleAnalysis, ARecordWithoutSpansHasNoRows)
{
    const Table table =
        SushiProfiler::DeviceIdleAnalysis().run(SushiProfilerTests::analysis_record(), {});

    EXPECT_TRUE(table.rows.empty());
}
