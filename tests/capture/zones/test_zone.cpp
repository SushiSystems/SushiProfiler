/**************************************************************************/
/* test_zone.cpp                                                          */
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
 * @file test_zone.cpp
 * @brief Tests that a scoped zone writes one complete event, and nothing when it must not.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#include <SushiProfiler/zones/zone.hpp>

#include "recording_fixture.hpp"

#include <SushiProfiler/zones/macros.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

using SushiProfiler::Collector;
using SushiProfiler::EventKind;
using SushiProfiler::ScopedLane;
using SushiProfiler::ScopedZone;
using SushiProfiler::SiteFilter;
using SushiProfiler::SiteInfo;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief The zone tests, on one thread.
    class Small_Zone : public SushiProfilerTests::RecordingFixture
    {
    };

    static_assert(std::is_empty_v<ScopedZone<false>>, "a compiled-out zone holds nothing");
    static_assert(std::is_empty_v<ScopedLane<false>>, "a compiled-out lane holds nothing");
} // namespace

TEST_F(Small_Zone, WritesOneCompleteEvent)
{
    const std::uint32_t line = __LINE__ + 2U;
    {
        SP_ZONE_AT(3, 1, "load", nullptr);
    }
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Zone);
    EXPECT_LE(events[0].begin, events[0].end);
    const SiteInfo info = Collector::instance().sites().info(SushiProfiler::site_of(events[0]));
    EXPECT_EQ(info.name, "load");
    EXPECT_EQ(info.line, line);
    EXPECT_EQ(info.level, 1U);
}

TEST_F(Small_Zone, NestedZonesNestByTime)
{
    {
        SP_ZONE_AT(3, 1, "outer", nullptr);
        {
            SP_ZONE_AT(3, 1, "inner", nullptr);
        }
    }
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    const TraceEvent& inner = events[0];
    const TraceEvent& outer = events[1];
    EXPECT_LE(outer.begin, inner.begin);
    EXPECT_LE(inner.end, outer.end);
}

TEST_F(Small_Zone, ANamedZoneCarriesItsName)
{
    const SushiProfiler::NameID name = Collector::instance().names().intern("conv_3");
    {
        SP_ZONE_NAMED_AT(3, 1, "node", nullptr, name);
    }
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, name);
}

TEST_F(Small_Zone, ADisabledSiteWritesNothing)
{
    SiteFilter filter;
    filter.groups = {"Data"};
    Collector::instance().apply(filter);
    {
        SP_ZONE_AT(3, 1, "run", "Device");
    }

    EXPECT_TRUE(drain_mine().empty());
}

TEST_F(Small_Zone, AZoneClosedAfterTheCollectorClosesWritesNothing)
{
    {
        SP_ZONE_AT(3, 1, "cut", nullptr);
        Collector::instance().close();
    }

    EXPECT_TRUE(drain_mine().empty());
}

TEST_F(Small_Zone, ACompiledOutZoneIsEmpty)
{
    {
        SP_ZONE_AT(1, 3, "detail", nullptr);
    }

    EXPECT_TRUE(drain_mine().empty());
}
