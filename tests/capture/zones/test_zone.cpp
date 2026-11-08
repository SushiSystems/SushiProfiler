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
 * @file test_zone.cpp
 * @brief Tests that a scoped zone writes one complete event, and nothing when it must not.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#include <SushiProfiler/zones/zone.hpp>

#include "recording_fixture.hpp"

#include <SushiProfiler/clock/manual_tick_source.hpp>
#include <SushiProfiler/collector/collector_settings.hpp>
#include <SushiProfiler/collector/drain_batch.hpp>
#include <SushiProfiler/sites/site.hpp>
#include <SushiProfiler/zones/macros.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::DrainBatch;
using SushiProfiler::EventKind;
using SushiProfiler::ManualTickSource;
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

    /// @brief The site of the zones recorded into a collector the test owns.
    constexpr SushiProfiler::Site K_OWNED_SITE{"load", nullptr, __FILE__, __LINE__, 1U,
                                               SushiProfiler::SiteRole::Zone,
                                               SushiProfiler::CounterOp::Add, nullptr};

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

TEST(Small_ZoneOfAGivenCollector, RecordsIntoItWithTheStampsOfItsClock)
{
    ManualTickSource clock(100U);
    Collector collector(clock);
    ASSERT_TRUE(collector.open(CollectorSettings{}));
    SushiProfiler::SiteHandle handle{&K_OWNED_SITE};

    {
        const ScopedZone<true> zone(collector, handle);
        clock.advance(25U);
    }

    DrainBatch batch;
    collector.drain(batch);
    ASSERT_EQ(batch.threads.size(), 1U);
    ASSERT_EQ(batch.threads[0].events.size(), 1U);
    EXPECT_EQ(batch.threads[0].events[0].begin, 100U);
    EXPECT_EQ(batch.threads[0].events[0].end, 125U);
}

TEST(Small_ZoneOfAGivenCollector, WritesNothingWhileItIsClosed)
{
    const ManualTickSource clock;
    Collector collector(clock);
    SushiProfiler::SiteHandle handle{&K_OWNED_SITE};

    {
        const ScopedZone<true> zone(collector, handle);
    }

    DrainBatch batch;
    collector.drain(batch);
    EXPECT_TRUE(batch.threads.empty());
}
