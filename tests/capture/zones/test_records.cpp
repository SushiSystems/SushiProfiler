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
 * @file test_records.cpp
 * @brief Tests the one-shot records and the lane: their kinds, fields and sites.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#include <SushiProfiler/zones/records.hpp>

#include "recording_fixture.hpp"

#include <SushiProfiler/zones/macros.hpp>
#include <SushiProfiler/zones/zone.hpp>

#include <gtest/gtest.h>

#include <cstdint>

using SushiProfiler::CounterOp;
using SushiProfiler::EventKind;
using SushiProfiler::NameID;
using SushiProfiler::Site;
using SushiProfiler::SiteHandle;
using SushiProfiler::SiteInfo;
using SushiProfiler::SiteRole;

namespace
{
    /// @brief The record tests, on one thread and a collector of their own.
    class Small_Records : public SushiProfilerTests::OwnedRecordingFixture
    {
    };

    /// @brief The record tests that go through the macros and so reach the process-wide collector.
    class Medium_Records : public SushiProfilerTests::RecordingFixture
    {
    };

    /// @brief A counter site that takes the largest value it is given, in items.
    constexpr Site K_COUNTER_SITE{"queue", nullptr, "test_records.cpp", 1U, 1U, SiteRole::Counter,
                                  CounterOp::Max, "items"};

    /// @brief A value site.
    constexpr Site K_VALUE_SITE{"loss", nullptr, "test_records.cpp", 1U, 1U, SiteRole::Value,
                                CounterOp::Add, nullptr};

    /// @brief A zone site in the group "Device".
    constexpr Site K_ZONE_SITE{"kernel", "Device", "test_records.cpp", 1U, 1U, SiteRole::Zone,
                               CounterOp::Add, nullptr};

    /// @brief An instant site.
    constexpr Site K_INSTANT_SITE{"fence", nullptr, "test_records.cpp", 1U, 1U, SiteRole::Instant,
                                  CounterOp::Add, nullptr};

    /// @brief A flow site.
    constexpr Site K_FLOW_SITE{"frame", nullptr, "test_records.cpp", 1U, 1U, SiteRole::Flow,
                               CounterOp::Add, nullptr};
} // namespace

TEST_F(Small_Records, ACounterKeepsItsValue)
{
    SiteHandle handle{&K_COUNTER_SITE};
    SushiProfiler::record_counter(collector, handle, 42.5);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Counter);
    EXPECT_EQ(SushiProfiler::value_of(events[0].end), 42.5);
}

TEST_F(Small_Records, ACounterSiteKeepsItsOperationAndUnit)
{
    SiteHandle handle{&K_COUNTER_SITE};
    SushiProfiler::record_counter(collector, handle, 42.5);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    const SiteInfo info = collector.sites().info(SushiProfiler::site_of(events[0]));
    EXPECT_EQ(info.op, CounterOp::Max);
    EXPECT_EQ(info.unit, "items");
    EXPECT_EQ(info.role, SiteRole::Counter);
}

TEST_F(Small_Records, AValueKeepsItsSample)
{
    SiteHandle handle{&K_VALUE_SITE};
    SushiProfiler::record_value(collector, handle, -0.25);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Value);
    EXPECT_EQ(SushiProfiler::value_of(events[0].end), -0.25);
}

TEST_F(Small_Records, AnExternalZoneKeepsTheCallersTimes)
{
    SiteHandle handle{&K_ZONE_SITE};
    SushiProfiler::record_zone_at(collector, handle, 1U, 5'000'000'001U);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::ExternalZone);
    EXPECT_EQ(events[0].begin, 1U);
    EXPECT_EQ(events[0].end, 5'000'000'001U);
}

TEST_F(Small_Records, AnExternalInstantKeepsTheCallersTime)
{
    SiteHandle handle{&K_INSTANT_SITE};
    SushiProfiler::record_instant_at(collector, handle, 77U);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::ExternalInstant);
    EXPECT_EQ(events[0].begin, 77U);
}

TEST_F(Small_Records, AFlowBeginCarriesItsId)
{
    SiteHandle handle{&K_FLOW_SITE};
    SushiProfiler::record_flow(collector, handle, EventKind::FlowBegin, 7U);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::FlowBegin);
    EXPECT_EQ(events[0].end, 7U);
}

TEST_F(Small_Records, AFlowStepCarriesItsId)
{
    SiteHandle handle{&K_FLOW_SITE};
    SushiProfiler::record_flow(collector, handle, EventKind::FlowStep, 7U);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::FlowStep);
    EXPECT_EQ(events[0].end, 7U);
}

TEST_F(Small_Records, AFlowEndCarriesItsId)
{
    SiteHandle handle{&K_FLOW_SITE};
    SushiProfiler::record_flow(collector, handle, EventKind::FlowEnd, 7U);
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::FlowEnd);
    EXPECT_EQ(events[0].end, 7U);
}

TEST_F(Small_Records, ALaneBracketsItsEvents)
{
    SiteHandle handle{&K_INSTANT_SITE};
    const NameID lane = collector.names().intern("worker");
    {
        const SushiProfiler::ScopedLane<true> scope(collector, lane);
        SushiProfiler::record_instant(collector, handle);
    }
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 3U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::LaneBegin);
    EXPECT_EQ(events[0].name, lane);
    EXPECT_EQ(SushiProfiler::kind_of(events[1]), EventKind::Instant);
    EXPECT_EQ(SushiProfiler::kind_of(events[2]), EventKind::LaneEnd);
    EXPECT_EQ(events[2].name, lane);
}

TEST_F(Medium_Records, ACompiledOutRecordWritesNothing)
{
    SP_COUNTER_AT(1, 2, "detail", "", Add, 1.0);
    SP_INSTANT_AT(0, 1, "off");
    {
        SP_LANE_AT(0, 1U);
    }

    EXPECT_TRUE(drain_mine().empty());
}
