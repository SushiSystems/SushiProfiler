/**************************************************************************/
/* test_site_registry.cpp                                                 */
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
 * @file test_site_registry.cpp
 * @brief Tests that sites register once, copy their strings and follow the run-time filter.
 * @author Mustafa Garip
 * @see tests/capture/sites/README.md
 */

#include <SushiProfiler/sites/site_registry.hpp>

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <thread>
#include <vector>

using SushiProfiler::CounterOp;
using SushiProfiler::Site;
using SushiProfiler::SiteFilter;
using SushiProfiler::SiteHandle;
using SushiProfiler::SiteID;
using SushiProfiler::SiteInfo;
using SushiProfiler::SiteRegistry;
using SushiProfiler::SiteRole;

namespace
{
    /// @brief Builds a zone site of @p level in @p group.
    constexpr Site zone_site(const char* name, const char* group, std::uint8_t level)
    {
        return Site{name, group, "test_site_registry.cpp", 1U, level, SiteRole::Zone,
                    CounterOp::Add, nullptr};
    }
} // namespace

TEST(Small_SiteRegistry, OneSiteGetsOneIdForEveryHandleCall)
{
    static constexpr Site site = zone_site("load", nullptr, 1U);
    SiteHandle handle{&site};
    SiteRegistry registry;

    const SiteID first = registry.id_of(handle);

    EXPECT_NE(first, 0U);
    EXPECT_EQ(registry.id_of(handle), first);
    EXPECT_EQ(registry.size(), 1U);
}

TEST(Small_SiteRegistry, TwoSitesWithEqualTextAreTwoSites)
{
    static constexpr Site first_site = zone_site("load", nullptr, 1U);
    static constexpr Site second_site = zone_site("load", nullptr, 1U);
    SiteHandle first{&first_site};
    SiteHandle second{&second_site};
    SiteRegistry registry;

    EXPECT_NE(registry.id_of(first), registry.id_of(second));
}

TEST(Small_SiteRegistry, TheFilterDisablesALevelAboveIt)
{
    static constexpr Site coarse_site = zone_site("step", nullptr, 1U);
    static constexpr Site fine_site = zone_site("inner", nullptr, 2U);
    SiteHandle coarse{&coarse_site};
    SiteHandle fine{&fine_site};
    SiteRegistry registry;
    SiteFilter filter;
    filter.level = 1U;
    registry.apply(filter);

    EXPECT_TRUE(registry.enabled(registry.id_of(coarse)));
    EXPECT_FALSE(registry.enabled(registry.id_of(fine)));
}

TEST(Small_SiteRegistry, TheFilterKeepsOnlyNamedGroupsAndUngroupedSites)
{
    static constexpr Site data_site = zone_site("read", "Data", 1U);
    static constexpr Site device_site = zone_site("run", "Device", 1U);
    static constexpr Site plain_site = zone_site("plain", nullptr, 1U);
    SiteHandle data{&data_site};
    SiteHandle device{&device_site};
    SiteHandle plain{&plain_site};
    SiteRegistry registry;
    SiteFilter filter;
    filter.groups = {"Data"};
    registry.apply(filter);

    EXPECT_TRUE(registry.enabled(registry.id_of(data)));
    EXPECT_FALSE(registry.enabled(registry.id_of(device)));
    EXPECT_TRUE(registry.enabled(registry.id_of(plain)));
}

TEST(Small_SiteRegistry, AFilterAppliesToSitesRegisteredLater)
{
    static constexpr Site fine_site = zone_site("late", nullptr, 3U);
    SiteHandle fine{&fine_site};
    SiteRegistry registry;
    SiteFilter filter;
    filter.level = 2U;
    registry.apply(filter);

    EXPECT_FALSE(registry.enabled(registry.id_of(fine)));

    filter.level = 3U;
    registry.apply(filter);

    EXPECT_TRUE(registry.enabled(registry.id_of(fine)));
}

TEST(Small_SiteRegistry, ASiteBeyondCapacityIsCountedNotRecorded)
{
    static constexpr Site sites[3] = {zone_site("a", nullptr, 1U), zone_site("b", nullptr, 1U),
                                      zone_site("c", nullptr, 1U)};
    SiteHandle first{&sites[0]};
    SiteHandle second{&sites[1]};
    SiteHandle third{&sites[2]};
    SiteRegistry registry(2U);

    EXPECT_NE(registry.id_of(first), 0U);
    EXPECT_NE(registry.id_of(second), 0U);
    EXPECT_EQ(registry.id_of(third), 0U);
    EXPECT_EQ(registry.id_of(third), 0U);
    EXPECT_FALSE(registry.enabled(0U));
    EXPECT_EQ(registry.refused(), 1U);
}

TEST(Small_SiteRegistry, InfoCopiesTheStrings)
{
    char name[16] = "decode";
    char group[16] = "Data";
    const Site site{name, group, "file.cpp", 42U, 2U, SiteRole::Counter, CounterOp::Max, "px"};
    SiteHandle handle{&site};
    SiteRegistry registry;
    const SiteID id = registry.id_of(handle);
    std::memcpy(name, "overwritten", sizeof("overwritten"));
    std::memcpy(group, "Other", sizeof("Other"));

    const SiteInfo info = registry.info(id);

    EXPECT_EQ(info.name, "decode");
    EXPECT_EQ(info.group, "Data");
    EXPECT_EQ(info.file, "file.cpp");
    EXPECT_EQ(info.unit, "px");
    EXPECT_EQ(info.line, 42U);
    EXPECT_EQ(info.level, 2U);
    EXPECT_EQ(info.role, SiteRole::Counter);
    EXPECT_EQ(info.op, CounterOp::Max);
}

TEST(Medium_SiteRegistry, ConcurrentFirstUseGivesOneId)
{
    static constexpr Site site = zone_site("shared", nullptr, 1U);
    SiteHandle handle{&site};
    SiteRegistry registry;
    std::vector<SiteID> ids(8U, 0U);
    std::vector<std::thread> threads;
    for (std::size_t index = 0U; index < ids.size(); ++index)
        threads.emplace_back([&, index] { ids[index] = registry.id_of(handle); });
    for (std::thread& thread : threads)
        thread.join();

    for (const SiteID id : ids)
        EXPECT_EQ(id, ids.front());
    EXPECT_EQ(registry.size(), 1U);
}
