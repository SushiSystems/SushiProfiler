/**************************************************************************/
/* test_named_lock_wait.cpp                                               */
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
 * @file test_named_lock_wait.cpp
 * @brief Tests the lock-wait macro whose lock a condition variable can wait on.
 * @author Mustafa Garip
 * @see tests/capture/lock_wait/README.md
 */

#include <SushiProfiler/lock_wait/named_lock_wait.hpp>

#include "scripted_mutex.hpp"

#include <SushiProfiler/clock/manual_tick_source.hpp>
#include <SushiProfiler/collector/collector.hpp>

#include <gtest/gtest.h>

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>

using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::DrainBatch;
using SushiProfiler::EventKind;
using SushiProfiler::ManualTickSource;
using SushiProfiler::SiteInfo;
using SushiProfiler::SiteRole;
using SushiProfiler::ThreadDrain;
using SushiProfiler::TraceEvent;
using SushiProfilerTests::ScriptedMutex;

namespace
{
    /// @brief The length of the wait a scripted holder imposes, in nanoseconds.
    constexpr std::uint64_t K_WAIT_NS = 250U;

    /// @brief The lock wait site the function tests record at.
    constexpr SushiProfiler::Site K_SITE{"queue", "Lock wait", "test_named_lock_wait.cpp",
                                         1U, 1U, SiteRole::LockWait,
                                         SushiProfiler::CounterOp::Add, nullptr};

    /// @brief Returns what @p collector holds for the calling thread, draining it.
    std::vector<TraceEvent> drain_mine(Collector& collector)
    {
        DrainBatch batch;
        collector.drain(batch);
        for (ThreadDrain& thread : batch.threads)
        {
            if (thread.thread_index == collector.index_of_thread())
                return std::move(thread.events);
        }
        return {};
    }

    /// @brief Gives each test its own open collector on a manual clock that starts at 100 ns.
    class Small_NamedLockWait : public ::testing::Test
    {
        protected:
            void SetUp() override
            {
                ASSERT_TRUE(collector.open(CollectorSettings{}));
            }

            ManualTickSource clock{100U};
            Collector collector{clock};
            SushiProfiler::SiteHandle handle{&K_SITE};
    };
} // namespace

TEST_F(Small_NamedLockWait, AnUncontendedLockWritesNothing)
{
    ScriptedMutex mutex(false);
    std::unique_lock<ScriptedMutex> lock(mutex, std::defer_lock);

    SushiProfiler::lock_with_wait<true>(collector, lock, handle);

    EXPECT_TRUE(drain_mine(collector).empty());
}

TEST_F(Small_NamedLockWait, AnUncontendedLockIsHeldOnReturn)
{
    ScriptedMutex mutex(false);
    std::unique_lock<ScriptedMutex> lock(mutex, std::defer_lock);

    SushiProfiler::lock_with_wait<true>(collector, lock, handle);

    EXPECT_TRUE(lock.owns_lock());
}

TEST_F(Small_NamedLockWait, AContendedLockIsHeldOnReturn)
{
    ScriptedMutex mutex(true, &clock, K_WAIT_NS);
    std::unique_lock<ScriptedMutex> lock(mutex, std::defer_lock);

    SushiProfiler::lock_with_wait<true>(collector, lock, handle);

    EXPECT_TRUE(lock.owns_lock());
}

TEST_F(Small_NamedLockWait, AContendedLockWritesOneZoneAsLongAsTheWait)
{
    ScriptedMutex mutex(true, &clock, K_WAIT_NS);
    std::unique_lock<ScriptedMutex> lock(mutex, std::defer_lock);

    SushiProfiler::lock_with_wait<true>(collector, lock, handle);
    const std::vector<TraceEvent> events = drain_mine(collector);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Zone);
    EXPECT_EQ(events[0].end - events[0].begin, K_WAIT_NS);
}

TEST_F(Small_NamedLockWait, TheWaitZoneSitsAtALockWaitSite)
{
    ScriptedMutex mutex(true, &clock, K_WAIT_NS);
    std::unique_lock<ScriptedMutex> lock(mutex, std::defer_lock);

    SushiProfiler::lock_with_wait<true>(collector, lock, handle);
    const std::vector<TraceEvent> events = drain_mine(collector);

    ASSERT_EQ(events.size(), 1U);
    const SiteInfo info = collector.sites().info(SushiProfiler::site_of(events[0]));
    EXPECT_EQ(info.name, "queue");
    EXPECT_EQ(info.role, SiteRole::LockWait);
}

TEST_F(Small_NamedLockWait, ACompiledOutLockStillLocksAndWritesNothing)
{
    ScriptedMutex mutex(true, &clock, K_WAIT_NS);
    std::unique_lock<ScriptedMutex> lock(mutex, std::defer_lock);
    SushiProfiler::SiteSlot<false> slot{nullptr};

    SushiProfiler::lock_with_wait<false>(collector, lock, slot);

    EXPECT_TRUE(lock.owns_lock());
    EXPECT_TRUE(drain_mine(collector).empty());
}

TEST_F(Small_NamedLockWait, TheMacroWritesOneLockWaitZoneWhenTheMutexIsHeld)
{
    ScriptedMutex mutex(true);
    {
        SP_LOCK_WAIT_NAMED_IN_AT(collector, 1, lock, "queue", mutex);

        EXPECT_TRUE(lock.owns_lock());
    }

    const std::vector<TraceEvent> events = drain_mine(collector);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Zone);
    const SiteInfo info = collector.sites().info(SushiProfiler::site_of(events[0]));
    EXPECT_EQ(info.role, SiteRole::LockWait);
    EXPECT_EQ(info.name, "queue");
}

TEST_F(Small_NamedLockWait, TheMacroAtLevelZeroStillLocks)
{
    ScriptedMutex mutex(false);
    {
        SP_LOCK_WAIT_NAMED_IN_AT(collector, 0, lock, "queue", mutex);

        EXPECT_TRUE(lock.owns_lock());
        EXPECT_TRUE(mutex.locked());
    }

    EXPECT_TRUE(drain_mine(collector).empty());
}

TEST_F(Small_NamedLockWait, TheLockPassesToAConditionVariableWait)
{
    ScriptedMutex mutex(false);
    std::condition_variable_any ready;
    const bool flag = true;
    {
        SP_LOCK_WAIT_NAMED_IN_AT(collector, 1, lock, "state", mutex);

        ready.wait(lock, [&] { return flag; });

        EXPECT_TRUE(lock.owns_lock());
        EXPECT_TRUE(mutex.locked());
    }
}
