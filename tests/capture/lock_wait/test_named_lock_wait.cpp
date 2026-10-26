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
 * @file test_named_lock_wait.cpp
 * @brief Tests the lock-wait macro whose lock a condition variable can wait on.
 * @author Mustafa Garip
 * @see tests/capture/lock_wait/README.md
 */

#include <SushiProfiler/lock_wait/named_lock_wait.hpp>

#include <SushiProfiler/collector/collector.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::DrainBatch;
using SushiProfiler::EventKind;
using SushiProfiler::SiteInfo;
using SushiProfiler::SiteRole;
using SushiProfiler::ThreadDrain;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief Opens the collector before each test, and closes and empties it after.
    class NamedLockWaitTest : public ::testing::Test
    {
        protected:
            void SetUp() override
            {
                close_and_empty();
                ASSERT_TRUE(Collector::instance().open(CollectorSettings{}));
            }

            void TearDown() override
            {
                close_and_empty();
            }

            /// @brief Returns what the calling thread wrote since the last drain.
            static std::vector<TraceEvent> drain_mine()
            {
                DrainBatch batch;
                Collector::instance().drain(batch);
                for (ThreadDrain& thread : batch.threads)
                {
                    if (thread.thread_index == Collector::thread_index())
                        return std::move(thread.events);
                }
                return {};
            }

        private:
            /// @brief Closes the collector and discards what its rings hold.
            static void close_and_empty()
            {
                Collector::instance().close();
                DrainBatch discarded;
                Collector::instance().drain(discarded);
            }
    };

    /// @brief The tests on one thread.
    class Small_NamedLockWait : public NamedLockWaitTest
    {
    };

    /// @brief The tests with a second thread.
    class Medium_NamedLockWait : public NamedLockWaitTest
    {
    };
} // namespace

TEST_F(Small_NamedLockWait, AnUncontendedLockWritesNothing)
{
    std::mutex mutex;
    {
        SP_LOCK_WAIT_NAMED_AT(1, lock, "queue", mutex);

        EXPECT_TRUE(lock.owns_lock());
    }

    EXPECT_TRUE(drain_mine().empty());
    EXPECT_TRUE(mutex.try_lock());
    mutex.unlock();
}

TEST_F(Small_NamedLockWait, ACompiledOutLockStillLocks)
{
    std::mutex mutex;
    {
        SP_LOCK_WAIT_NAMED_AT(0, lock, "queue", mutex);

        EXPECT_TRUE(lock.owns_lock());
        EXPECT_FALSE(mutex.try_lock());
    }

    EXPECT_TRUE(drain_mine().empty());
}

TEST_F(Medium_NamedLockWait, AContendedLockRecordsOneLockWaitZone)
{
    std::mutex mutex;
    std::atomic<bool> held{false};
    std::thread holder(
        [&]
        {
            const std::lock_guard<std::mutex> guard(mutex);
            held.store(true, std::memory_order_release);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        });
    while (!held.load(std::memory_order_acquire))
        std::this_thread::yield();
    {
        SP_LOCK_WAIT_NAMED_AT(1, lock, "queue", mutex);
    }
    holder.join();
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Zone);
    const SiteInfo info =
        Collector::instance().sites().info(SushiProfiler::site_of(events[0]));
    EXPECT_EQ(info.role, SiteRole::LockWait);
    EXPECT_EQ(info.name, "queue");
}

TEST_F(Medium_NamedLockWait, TheLockPassesToAConditionVariableWait)
{
    std::mutex mutex;
    std::condition_variable ready;
    bool flag = false;
    std::thread setter(
        [&]
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            {
                const std::lock_guard<std::mutex> guard(mutex);
                flag = true;
            }
            ready.notify_one();
        });
    {
        SP_LOCK_WAIT_NAMED_AT(1, lock, "state", mutex);
        ready.wait(lock, [&] { return flag; });

        EXPECT_TRUE(flag);
        EXPECT_TRUE(lock.owns_lock());
    }
    setter.join();
}
