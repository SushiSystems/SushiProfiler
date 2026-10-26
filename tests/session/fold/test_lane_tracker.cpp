/**************************************************************************/
/* test_lane_tracker.cpp                                                  */
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
 * @file test_lane_tracker.cpp
 * @brief Tests opening, nesting and closing lanes, an unmatched end and a reset.
 * @author Mustafa Garip
 * @see tests/session/fold/README.md
 */

#include <SushiProfiler/fold/lane_tracker.hpp>

#include <gtest/gtest.h>

using SushiProfiler::EventKind;
using SushiProfiler::LaneTracker;
using SushiProfiler::NameID;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief Returns an event of @p kind naming @p name.
    TraceEvent event_of(EventKind kind, NameID name = 0U)
    {
        return TraceEvent{0U, 0U, SushiProfiler::pack_site(kind, 1U), name};
    }
} // namespace

TEST(Small_LaneTracker, GivesEachEventTheInnermostOpenLane)
{
    LaneTracker lanes;

    EXPECT_EQ(lanes.lane_of(event_of(EventKind::Zone)), 0U);
    EXPECT_EQ(lanes.lane_of(event_of(EventKind::LaneBegin, 5U)), 5U);
    EXPECT_EQ(lanes.lane_of(event_of(EventKind::Zone)), 5U);
    EXPECT_EQ(lanes.lane_of(event_of(EventKind::LaneBegin, 6U)), 6U);
    EXPECT_EQ(lanes.lane_of(event_of(EventKind::Counter)), 6U);
    EXPECT_EQ(lanes.open_lanes(), 2U);
    EXPECT_EQ(lanes.lane_of(event_of(EventKind::LaneEnd, 6U)), 6U);
    EXPECT_EQ(lanes.lane_of(event_of(EventKind::Zone)), 5U);
    EXPECT_EQ(lanes.lane_of(event_of(EventKind::LaneEnd, 5U)), 5U);
    EXPECT_EQ(lanes.lane_of(event_of(EventKind::Zone)), 0U);
    EXPECT_EQ(lanes.unmatched(), 0U);
}

TEST(Small_LaneTracker, AnEndOfALaneNotOpenIsUnmatched)
{
    LaneTracker lanes;
    static_cast<void>(lanes.lane_of(event_of(EventKind::LaneBegin, 5U)));

    EXPECT_EQ(lanes.lane_of(event_of(EventKind::LaneEnd, 9U)), 5U);
    EXPECT_EQ(lanes.unmatched(), 1U);
    EXPECT_EQ(lanes.open_lanes(), 1U);
}

TEST(Small_LaneTracker, AnInnerLaneClosedFirstLeavesTheOthersOpen)
{
    LaneTracker lanes;
    static_cast<void>(lanes.lane_of(event_of(EventKind::LaneBegin, 5U)));
    static_cast<void>(lanes.lane_of(event_of(EventKind::LaneBegin, 6U)));

    static_cast<void>(lanes.lane_of(event_of(EventKind::LaneEnd, 5U)));

    EXPECT_EQ(lanes.lane_of(event_of(EventKind::Zone)), 6U);
    EXPECT_EQ(lanes.open_lanes(), 1U);
}

TEST(Small_LaneTracker, ResetClosesEveryLane)
{
    LaneTracker lanes;
    static_cast<void>(lanes.lane_of(event_of(EventKind::LaneBegin, 5U)));

    lanes.reset();

    EXPECT_EQ(lanes.open_lanes(), 0U);
    EXPECT_EQ(lanes.lane_of(event_of(EventKind::Zone)), 0U);
}
