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

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

using SushiProfiler::Collector;
using SushiProfiler::EventKind;
using SushiProfiler::ManualTickSource;
using SushiProfiler::ScopedLane;
using SushiProfiler::ScopedZone;
using SushiProfiler::SiteFilter;
using SushiProfiler::SiteHandle;
using SushiProfiler::SiteInfo;

namespace
{
    /// @brief The zone tests, on one thread and a collector of their own that starts at 100 ns.
    class Small_Zone : public SushiProfilerTests::OwnedRecordingFixture
    {
    };

    /// @brief The site of an ungrouped zone at level 1, claiming line 7.
    constexpr SushiProfiler::Site K_SITE{"load", nullptr, "test_zone.cpp", 7U, 1U,
                                         SushiProfiler::SiteRole::Zone,
                                         SushiProfiler::CounterOp::Add, nullptr};

    /// @brief The site of a zone in the group "Device".
    constexpr SushiProfiler::Site K_DEVICE_SITE{"run", "Device", "test_zone.cpp", 8U, 1U,
                                                SushiProfiler::SiteRole::Zone,
                                                SushiProfiler::CounterOp::Add, nullptr};

    static_assert(std::is_empty_v<ScopedZone<false>>, "a compiled-out zone holds nothing");
    static_assert(std::is_empty_v<ScopedLane<false>>, "a compiled-out lane holds nothing");
} // namespace

TEST_F(Small_Zone, WritesOneCompleteEvent)
{
    SiteHandle handle{&K_SITE};
    {
        const ScopedZone<true> zone(collector, handle);
        clock.advance(25U);
    }
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Zone);
    EXPECT_EQ(events[0].begin, 100U);
    EXPECT_EQ(events[0].end, 125U);
}

TEST_F(Small_Zone, TheEventCarriesTheSiteOfItsZone)
{
    SiteHandle handle{&K_SITE};
    {
        const ScopedZone<true> zone(collector, handle);
    }
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    const SiteInfo info = collector.sites().info(SushiProfiler::site_of(events[0]));
    EXPECT_EQ(info.name, "load");
    EXPECT_EQ(info.line, 7U);
    EXPECT_EQ(info.level, 1U);
}

TEST_F(Small_Zone, NestedZonesNestByTime)
{
    SiteHandle outer_handle{&K_SITE};
    SiteHandle inner_handle{&K_SITE};
    {
        const ScopedZone<true> outer(collector, outer_handle);
        clock.advance(10U);
        {
            const ScopedZone<true> inner(collector, inner_handle);
            clock.advance(20U);
        }
        clock.advance(30U);
    }
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].begin, 110U);
    EXPECT_EQ(events[0].end, 130U);
    EXPECT_EQ(events[1].begin, 100U);
    EXPECT_EQ(events[1].end, 160U);
}

TEST_F(Small_Zone, ANamedZoneCarriesItsName)
{
    SiteHandle handle{&K_SITE};
    const SushiProfiler::NameID name = collector.names().intern("conv_3");
    {
        const ScopedZone<true> zone(collector, handle, name);
    }
    const auto events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].name, name);
}

TEST_F(Small_Zone, ADisabledSiteWritesNothing)
{
    SiteHandle handle{&K_DEVICE_SITE};
    SiteFilter filter;
    filter.groups = {"Data"};
    collector.apply(filter);
    {
        const ScopedZone<true> zone(collector, handle);
    }

    EXPECT_TRUE(drain_mine().empty());
}

TEST_F(Small_Zone, AZoneClosedAfterTheCollectorClosesWritesNothing)
{
    SiteHandle handle{&K_SITE};
    {
        const ScopedZone<true> zone(collector, handle);
        collector.close();
    }

    EXPECT_TRUE(drain_mine().empty());
}

TEST(Small_ZoneOfAClosedCollector, WritesNothing)
{
    const ManualTickSource clock;
    Collector collector(clock);
    SiteHandle handle{&K_SITE};

    {
        const ScopedZone<true> zone(collector, handle);
    }

    SushiProfiler::DrainBatch batch;
    collector.drain(batch);
    EXPECT_TRUE(batch.threads.empty());
}
