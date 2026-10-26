/**************************************************************************/
/* test_records.cpp                                                       */
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
 * @file test_records.cpp
 * @brief Tests the one-shot records and the lane: their kinds, fields and sites.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#include <SushiProfiler/zones/records.hpp>

#include "recording_fixture.hpp"

#include <SushiProfiler/zones/macros.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using SushiProfiler::Collector;
using SushiProfiler::CounterOp;
using SushiProfiler::EventKind;
using SushiProfiler::NameID;
using SushiProfiler::Site;
using SushiProfiler::SiteHandle;
using SushiProfiler::SiteInfo;
using SushiProfiler::SiteRole;

namespace
{
    /// @brief The record tests, on one thread.
    class Small_Records : public SushiProfilerTests::RecordingFixture
    {
    };
} // namespace

TEST_F(Small_Records, ACounterKeepsItsOperationAndValue)
{
    SP_COUNTER_AT(3, 1, "queue", "items", Max, 42.5);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Counter);
    EXPECT_EQ(SushiProfiler::value_of(events[0].end), 42.5);
    const SiteInfo info = Collector::instance().sites().info(SushiProfiler::site_of(events[0]));
    EXPECT_EQ(info.op, CounterOp::Max);
    EXPECT_EQ(info.unit, "items");
    EXPECT_EQ(info.role, SiteRole::Counter);
}

TEST_F(Small_Records, AValueKeepsItsSample)
{
    SP_VALUE_AT(3, 1, "loss", "", -0.25);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Value);
    EXPECT_EQ(SushiProfiler::value_of(events[0].end), -0.25);
}

TEST_F(Small_Records, AnExternalZoneKeepsTheCallersTimes)
{
    static constexpr Site site{"kernel", "Device", "test_records.cpp", 1U, 1U, SiteRole::Zone,
                               CounterOp::Add, nullptr};
    static SiteHandle handle{&site};
    SushiProfiler::record_zone_at(handle, 1U, 5'000'000'001U);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::ExternalZone);
    EXPECT_EQ(events[0].begin, 1U);
    EXPECT_EQ(events[0].end, 5'000'000'001U);
}

TEST_F(Small_Records, AnExternalInstantKeepsTheCallersTime)
{
    static constexpr Site site{"fence", nullptr, "test_records.cpp", 1U, 1U, SiteRole::Instant,
                               CounterOp::Add, nullptr};
    static SiteHandle handle{&site};
    SushiProfiler::record_instant_at(handle, 77U);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::ExternalInstant);
    EXPECT_EQ(events[0].begin, 77U);
}

TEST_F(Small_Records, FlowsCarryTheirId)
{
    SP_FLOW_AT(3, 1, "frame", FlowBegin, 7U);
    SP_FLOW_AT(3, 1, "frame", FlowStep, 7U);
    SP_FLOW_AT(3, 1, "frame", FlowEnd, 7U);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 3U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::FlowBegin);
    EXPECT_EQ(SushiProfiler::kind_of(events[1]), EventKind::FlowStep);
    EXPECT_EQ(SushiProfiler::kind_of(events[2]), EventKind::FlowEnd);
    for (const auto& event : events)
        EXPECT_EQ(event.end, 7U);
}

TEST_F(Small_Records, ALaneBracketsItsEvents)
{
    const NameID lane = Collector::instance().names().intern("worker");
    {
        SP_LANE_AT(3, lane);
        SP_INSTANT_AT(3, 1, "tick");
    }
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 3U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::LaneBegin);
    EXPECT_EQ(events[0].name, lane);
    EXPECT_EQ(SushiProfiler::kind_of(events[1]), EventKind::Instant);
    EXPECT_EQ(SushiProfiler::kind_of(events[2]), EventKind::LaneEnd);
    EXPECT_EQ(events[2].name, lane);
}

TEST_F(Small_Records, ACompiledOutRecordWritesNothing)
{
    SP_COUNTER_AT(1, 2, "detail", "", Add, 1.0);
    SP_INSTANT_AT(0, 1, "off");
    {
        SP_LANE_AT(0, 1U);
    }

    EXPECT_TRUE(drain_mine().empty());
}
