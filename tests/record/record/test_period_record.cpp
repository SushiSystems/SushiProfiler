/**************************************************************************/
/* test_period_record.cpp                                                 */
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
 * @file test_period_record.cpp
 * @brief Tests a period's duration and that equality reaches its trees, tags and spans.
 * @author Mustafa Garip
 * @see tests/record/record/README.md
 */

#include <SushiProfiler/record/period_record.hpp>

#include <gtest/gtest.h>

using SushiProfiler::PeriodRecord;
using SushiProfiler::ZoneNode;
using SushiProfiler::ZoneRun;
using SushiProfiler::ZoneTree;

TEST(Small_PeriodRecord, DurationIsEndLessBegin)
{
    PeriodRecord period;
    period.begin_ns = 1'000U;
    period.end_ns = 5'000'001'000U;

    EXPECT_EQ(period.duration_ns(), 5'000'000'000U);
}

TEST(Small_PeriodRecord, EqualityReachesTreesTagsAndSpans)
{
    PeriodRecord first;
    ZoneTree tree;
    tree.nodes.push_back(ZoneNode{1U, 0U, -1, 10U, 10U, 1U});
    first.zones.push_back(tree);
    first.tags.push_back({1U, 2U});
    PeriodRecord second = first;

    EXPECT_EQ(first, second);
    second.zones[0].nodes[0].calls = 2U;
    EXPECT_FALSE(first == second);
    second = first;
    second.spans.append({});
    EXPECT_FALSE(first == second);
    second = first;
    second.tags[0].value = 3U;
    EXPECT_FALSE(first == second);
}

TEST(Small_PeriodRecord, EqualityReachesEveryFieldOfAZoneRun)
{
    PeriodRecord first;
    first.runs.push_back(ZoneRun{1U, 2U, 3U, 4U, 5U, 6U, 7U});
    PeriodRecord second = first;

    EXPECT_EQ(first, second);
    second.runs[0].depth = 8U;
    EXPECT_FALSE(first == second);
    second = first;
    second.runs[0].begin_ns = 0U;
    EXPECT_FALSE(first == second);
    second = first;
    second.runs.clear();
    EXPECT_FALSE(first == second);
}
