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
 * @file test_dynamic_site_table.cpp
 * @brief Tests that run-time names get one stable site each, within the table's limit.
 * @author Mustafa Garip
 * @see tests/capture/sites/README.md
 */

#include <SushiProfiler/sites/dynamic_site_table.hpp>

#include <SushiProfiler/sites/site_registry.hpp>

#include <gtest/gtest.h>

#include <cstddef>
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

namespace
{
    /// @brief Gives @p table as many distinct names as it accepts.
    void fill_to_limit(DynamicSiteTable& table)
    {
        for (std::size_t index = 0U; index < DynamicSiteTable::K_MAX_NAMES; ++index)
            table.handle("name" + std::to_string(index), "", CounterOp::Add, 1U);
    }

    /// @brief Returns the handles @p calls lookups of @p names rotating names return in @p table.
    std::vector<SiteHandle*> handles_of_calls(DynamicSiteTable& table, std::size_t calls,
                                              std::size_t names)
    {
        std::vector<SiteHandle*> seen;
        for (std::size_t call = 0U; call < calls; ++call)
        {
            const std::string name = "shared" + std::to_string(call % names);
            seen.push_back(table.handle(name, "", CounterOp::Add, 1U));
        }
        return seen;
    }

    /// @brief Runs @p thread_count threads that each call handles_of_calls; returns what each saw.
    std::vector<std::vector<SiteHandle*>> handles_seen_by_threads(DynamicSiteTable& table,
                                                                  std::size_t thread_count,
                                                                  std::size_t calls,
                                                                  std::size_t names)
    {
        std::vector<std::vector<SiteHandle*>> seen(thread_count);
        std::vector<std::thread> threads;
        for (std::size_t thread = 0U; thread < thread_count; ++thread)
        {
            threads.emplace_back([&table, &seen, thread, calls, names]
                                 { seen[thread] = handles_of_calls(table, calls, names); });
        }
        for (std::thread& thread : threads)
            thread.join();
        return seen;
    }

    /// @brief Returns whether every list in @p lists equals the first.
    bool all_equal_to_first(const std::vector<std::vector<SiteHandle*>>& lists)
    {
        for (const std::vector<SiteHandle*>& list : lists)
        {
            if (list != lists.front())
                return false;
        }
        return true;
    }
} // namespace

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

TEST(Small_DynamicSiteTable, KeepsTheFirstUnitOfAName)
{
    DynamicSiteTable table(SiteRole::Counter);
    SiteHandle* const first = table.handle("depth", "items", CounterOp::Max, 1U);
    SiteHandle* const again = table.handle("depth", "bytes", CounterOp::Add, 2U);

    ASSERT_EQ(first, again);
    EXPECT_STREQ(first->site->unit, "items");
}

TEST(Small_DynamicSiteTable, KeepsTheFirstOperationOfAName)
{
    DynamicSiteTable table(SiteRole::Counter);
    SiteHandle* const first = table.handle("depth", "items", CounterOp::Max, 1U);
    SiteHandle* const again = table.handle("depth", "bytes", CounterOp::Add, 2U);

    ASSERT_EQ(first, again);
    EXPECT_EQ(first->site->op, CounterOp::Max);
}

TEST(Small_DynamicSiteTable, KeepsTheFirstLevelOfAName)
{
    DynamicSiteTable table(SiteRole::Counter);
    SiteHandle* const first = table.handle("depth", "items", CounterOp::Max, 1U);
    SiteHandle* const again = table.handle("depth", "bytes", CounterOp::Add, 2U);

    ASSERT_EQ(first, again);
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
    fill_to_limit(table);

    EXPECT_EQ(table.handle("one.too.many", "", CounterOp::Add, 1U), nullptr);
    EXPECT_EQ(table.refused(), 1U);
    EXPECT_NE(table.handle("name0", "", CounterOp::Add, 1U), nullptr);
    EXPECT_EQ(table.size(), DynamicSiteTable::K_MAX_NAMES);
}

TEST(Medium_DynamicSiteTable, ManyThreadsGetTheSameHandle)
{
    constexpr std::size_t K_THREADS = 8U;
    constexpr std::size_t K_CALLS = 1000U;
    constexpr std::size_t K_NAMES = 10U;
    DynamicSiteTable table(SiteRole::Counter);

    const std::vector<std::vector<SiteHandle*>> seen =
        handles_seen_by_threads(table, K_THREADS, K_CALLS, K_NAMES);

    EXPECT_EQ(table.size(), K_NAMES);
    EXPECT_TRUE(all_equal_to_first(seen));
}
