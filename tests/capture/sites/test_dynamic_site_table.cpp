/**************************************************************************/
/* test_dynamic_site_table.cpp                                            */
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
 * @file test_dynamic_site_table.cpp
 * @brief Tests that run-time names get one stable site each, within the table's limit.
 * @author Mustafa Garip
 * @see tests/capture/sites/README.md
 */

#include <SushiProfiler/sites/dynamic_site_table.hpp>

#include <SushiProfiler/sites/site_registry.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using SushiProfiler::CounterOp;
using SushiProfiler::DynamicSiteTable;
using SushiProfiler::SiteHandle;
using SushiProfiler::SiteInfo;
using SushiProfiler::SiteRegistry;
using SushiProfiler::SiteRole;

TEST(Small_DynamicSiteTable, ReturnsOneStableHandlePerName)
{
    DynamicSiteTable table(SiteRole::Counter);
    SiteHandle* const first = table.handle("frames", "", CounterOp::Add, 1U);
    SiteHandle* const again = table.handle("frames", "", CounterOp::Add, 1U);
    SiteHandle* const other = table.handle("bytes", "B", CounterOp::Add, 1U);

    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first, again);
    EXPECT_NE(first, other);
    EXPECT_EQ(table.size(), 2U);
}

TEST(Small_DynamicSiteTable, KeepsTheFirstUnitAndOperationOfAName)
{
    DynamicSiteTable table(SiteRole::Counter);
    SiteHandle* const first = table.handle("depth", "items", CounterOp::Max, 1U);
    SiteHandle* const again = table.handle("depth", "bytes", CounterOp::Add, 2U);

    ASSERT_EQ(first, again);
    EXPECT_STREQ(first->site->unit, "items");
    EXPECT_EQ(first->site->op, CounterOp::Max);
    EXPECT_EQ(first->site->level, 1U);
}

TEST(Small_DynamicSiteTable, CopiesTheNameSoATemporaryMayBeFreed)
{
    DynamicSiteTable table(SiteRole::Value);
    SiteHandle* handle = nullptr;
    {
        const std::string temporary = "queue.depth.0";
        handle = table.handle(temporary, "", CounterOp::Add, 1U);
    }
    ASSERT_NE(handle, nullptr);
    SiteRegistry registry;
    const SushiProfiler::SiteID id = registry.id_of(*handle);

    ASSERT_NE(id, 0U);
    const SiteInfo info = registry.info(id);
    EXPECT_EQ(info.name, "queue.depth.0");
    EXPECT_EQ(info.role, SiteRole::Value);
}

TEST(Small_DynamicSiteTable, RefusesNamesPastTheLimitAndCountsThem)
{
    DynamicSiteTable table(SiteRole::Counter);
    for (std::size_t index = 0U; index < DynamicSiteTable::K_MAX_NAMES; ++index)
    {
        ASSERT_NE(table.handle("name" + std::to_string(index), "", CounterOp::Add, 1U), nullptr);
    }

    EXPECT_EQ(table.handle("one.too.many", "", CounterOp::Add, 1U), nullptr);
    EXPECT_EQ(table.refused(), 1U);
    EXPECT_NE(table.handle("name0", "", CounterOp::Add, 1U), nullptr);
    EXPECT_EQ(table.size(), DynamicSiteTable::K_MAX_NAMES);
}

TEST(Medium_DynamicSiteTable, ManyThreadsGetTheSameHandle)
{
    DynamicSiteTable table(SiteRole::Counter);
    constexpr std::size_t K_THREADS = 8U;
    constexpr std::size_t K_NAMES = 10U;
    std::vector<std::vector<SiteHandle*>> seen(K_THREADS);
    std::vector<std::thread> threads;
    for (std::size_t thread = 0U; thread < K_THREADS; ++thread)
    {
        threads.emplace_back(
            [&table, &seen, thread]
            {
                for (std::size_t call = 0U; call < 1000U; ++call)
                {
                    const std::string name = "shared" + std::to_string(call % K_NAMES);
                    seen[thread].push_back(table.handle(name, "", CounterOp::Add, 1U));
                }
            });
    }
    for (std::thread& thread : threads)
        thread.join();

    EXPECT_EQ(table.size(), K_NAMES);
    for (std::size_t thread = 1U; thread < K_THREADS; ++thread)
        EXPECT_EQ(seen[thread], seen[0]);
}
