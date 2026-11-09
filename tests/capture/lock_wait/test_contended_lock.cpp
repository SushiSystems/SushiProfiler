/**************************************************************************/
/* test_contended_lock.cpp                                                */
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
 * @file test_contended_lock.cpp
 * @brief Tests that a lock records a wait zone only when the mutex is held by another.
 * @author Mustafa Garip
 * @see tests/capture/lock_wait/README.md
 */

#include <SushiProfiler/lock_wait/contended_lock.hpp>

#include "scripted_mutex.hpp"

#include <SushiProfiler/clock/manual_tick_source.hpp>
#include <SushiProfiler/collector/collector.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <utility>
#include <vector>

using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::ContendedLock;
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

    /// @brief The lock wait site the class tests record at.
    constexpr SushiProfiler::Site K_SITE{"queue", "Lock wait", "test_contended_lock.cpp",
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
    class Small_ContendedLock : public ::testing::Test
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

TEST_F(Small_ContendedLock, AnUncontendedLockWritesNothing)
{
    ScriptedMutex mutex(false);
    {
        const ContendedLock<ScriptedMutex> lock(collector, mutex, handle);
    }

    EXPECT_TRUE(drain_mine(collector).empty());
}

TEST_F(Small_ContendedLock, TheLockReleasesTheMutexAtScopeEnd)
{
    ScriptedMutex mutex(false);
    {
        const ContendedLock<ScriptedMutex> lock(collector, mutex, handle);
    }

    EXPECT_FALSE(mutex.locked());
}

TEST_F(Small_ContendedLock, UnlockReleasesTheMutex)
{
    ScriptedMutex mutex(false);
    ContendedLock<ScriptedMutex> lock(collector, mutex, handle);

    lock.unlock();

    EXPECT_FALSE(lock.owns_lock());
    EXPECT_FALSE(mutex.locked());
}

TEST_F(Small_ContendedLock, LockTakesTheMutexAgain)
{
    ScriptedMutex mutex(false);
    ContendedLock<ScriptedMutex> lock(collector, mutex, handle);
    lock.unlock();

    lock.lock();

    EXPECT_TRUE(lock.owns_lock());
    EXPECT_TRUE(mutex.locked());
}

TEST_F(Small_ContendedLock, AContendedLockWritesOneZone)
{
    ScriptedMutex mutex(true, &clock, K_WAIT_NS);
    {
        const ContendedLock<ScriptedMutex> lock(collector, mutex, handle);
    }

    const std::vector<TraceEvent> events = drain_mine(collector);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Zone);
}

TEST_F(Small_ContendedLock, TheWaitZoneLastsTheWaitOfTheHolder)
{
    ScriptedMutex mutex(true, &clock, K_WAIT_NS);
    {
        const ContendedLock<ScriptedMutex> lock(collector, mutex, handle);
    }

    const std::vector<TraceEvent> events = drain_mine(collector);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].end - events[0].begin, K_WAIT_NS);
}

TEST_F(Small_ContendedLock, TheWaitZoneSitsAtALockWaitSite)
{
    ScriptedMutex mutex(true, &clock, K_WAIT_NS);
    {
        const ContendedLock<ScriptedMutex> lock(collector, mutex, handle);
    }

    const std::vector<TraceEvent> events = drain_mine(collector);

    ASSERT_EQ(events.size(), 1U);
    const SiteInfo info = collector.sites().info(SushiProfiler::site_of(events[0]));
    EXPECT_EQ(info.name, "queue");
    EXPECT_EQ(info.group, "Lock wait");
    EXPECT_EQ(info.role, SiteRole::LockWait);
}

TEST_F(Small_ContendedLock, ACompiledOutLockStillLocksAndWritesNothing)
{
    ScriptedMutex mutex(true, &clock, K_WAIT_NS);
    SushiProfiler::SiteSlot<false> slot{nullptr};
    {
        const ContendedLock<ScriptedMutex, false> lock(collector, mutex, slot);

        EXPECT_TRUE(mutex.locked());
    }

    EXPECT_TRUE(drain_mine(collector).empty());
}

TEST_F(Small_ContendedLock, TheMacroWritesOneZoneAtALockWaitSiteWhenTheMutexIsHeld)
{
    ScriptedMutex mutex(true);
    {
        SP_LOCK_WAIT_IN_AT(collector, 1, "queue", mutex);
    }

    const std::vector<TraceEvent> events = drain_mine(collector);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Zone);
    const SiteInfo info = collector.sites().info(SushiProfiler::site_of(events[0]));
    EXPECT_EQ(info.name, "queue");
    EXPECT_EQ(info.role, SiteRole::LockWait);
}

TEST_F(Small_ContendedLock, TheMacroAtLevelZeroLocksAndWritesNothing)
{
    ScriptedMutex mutex(true);
    {
        SP_LOCK_WAIT_IN_AT(collector, 0, "queue", mutex);

        EXPECT_TRUE(mutex.locked());
    }

    EXPECT_TRUE(drain_mine(collector).empty());
}
