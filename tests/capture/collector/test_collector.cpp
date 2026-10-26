/**************************************************************************/
/* test_collector.cpp                                                     */
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
 * @file test_collector.cpp
 * @brief Tests the collector's gate, its thread rings, their retirement and the drain.
 * @author Mustafa Garip
 * @see tests/capture/collector/README.md
 */

#include <SushiProfiler/collector/collector.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <thread>
#include <vector>

using SushiProfiler::DrainBatch;
using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::ThreadDrain;
using SushiProfiler::ThreadRing;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief Opens the collector for each test and closes and empties it after.
    class CollectorTest : public ::testing::Test
    {
        protected:
            /// @brief Opens the collector with @p settings, closing any open one first.
            void reopen(const CollectorSettings& settings)
            {
                close_and_empty();
                ASSERT_TRUE(Collector::instance().open(settings));
            }

            void SetUp() override
            {
                reopen(CollectorSettings{});
            }

            void TearDown() override
            {
                close_and_empty();
            }

            /// @brief Closes the collector and discards what its rings hold.
            static void close_and_empty()
            {
                Collector::instance().close();
                DrainBatch discarded;
                Collector::instance().drain(discarded);
            }

            /// @brief Pushes @p count events numbered from 0 on the calling thread.
            static void push_events(std::uint64_t count)
            {
                ThreadRing* const ring = Collector::ring();
                ASSERT_NE(ring, nullptr);
                for (std::uint64_t value = 0U; value < count; ++value)
                {
                    TraceEvent event{};
                    event.begin = value;
                    ring->push(event);
                }
            }

            /// @brief Returns the drain of thread @p index in @p batch, or null.
            static const ThreadDrain* find(const DrainBatch& batch, std::uint32_t index)
            {
                for (const ThreadDrain& thread : batch.threads)
                {
                    if (thread.thread_index == index)
                        return &thread;
                }
                return nullptr;
            }
    };

    /// @brief The collector's tests that run on one thread.
    class Small_Collector : public CollectorTest
    {
    };

    /// @brief The collector's tests that start threads.
    class Medium_Collector : public CollectorTest
    {
    };
} // namespace

TEST_F(Small_Collector, OpensOnceAndRefusesASecondOpen)
{
    EXPECT_TRUE(Collector::instance().is_open());
    EXPECT_TRUE(Collector::recording());
    EXPECT_FALSE(Collector::instance().open(CollectorSettings{}));
}

TEST_F(Medium_Collector, ANewThreadOfAClosedCollectorGetsNoRing)
{
    Collector::instance().close();
    ThreadRing* ring_of_new_thread = reinterpret_cast<ThreadRing*>(1);
    std::thread([&] { ring_of_new_thread = Collector::ring(); }).join();

    EXPECT_FALSE(Collector::recording());
    EXPECT_EQ(ring_of_new_thread, nullptr);
}

TEST_F(Small_Collector, ADrainReturnsWhatThisThreadPushed)
{
    push_events(5U);
    DrainBatch batch;
    Collector::instance().drain(batch);

    const ThreadDrain* const mine = find(batch, Collector::thread_index());
    ASSERT_NE(mine, nullptr);
    ASSERT_EQ(mine->events.size(), 5U);
    EXPECT_EQ(mine->events[4].begin, 4U);
    EXPECT_FALSE(mine->retired);
}

TEST_F(Medium_Collector, DropsReachTheDrain)
{
    CollectorSettings settings;
    settings.ring_events = 64U;
    reopen(settings);
    std::uint32_t index = 0U;
    std::thread([&] {
        push_events(70U);
        index = Collector::thread_index();
    }).join();
    DrainBatch batch;
    Collector::instance().drain(batch);

    const ThreadDrain* const writer = find(batch, index);
    ASSERT_NE(writer, nullptr);
    EXPECT_EQ(writer->events.size(), 64U);
    EXPECT_EQ(writer->dropped, 6U);
}

TEST_F(Small_Collector, OpenDiscardsEventsFromBefore)
{
    push_events(3U);
    reopen(CollectorSettings{});
    DrainBatch batch;
    Collector::instance().drain(batch);

    const ThreadDrain* const mine = find(batch, Collector::thread_index());
    ASSERT_NE(mine, nullptr);
    EXPECT_TRUE(mine->events.empty());
}

TEST_F(Medium_Collector, ARegisteredThreadKeepsItsRingWhileClosed)
{
    Collector::instance().close();
    ThreadRing* ring = nullptr;
    std::size_t capacity = 0U;
    std::thread([&] {
        ASSERT_TRUE(Collector::register_this_thread(128U));
        ring = Collector::ring();
        capacity = ring != nullptr ? ring->capacity() : 0U;
    }).join();

    EXPECT_NE(ring, nullptr);
    EXPECT_EQ(capacity, 128U);
}

TEST_F(Small_Collector, NamedThreadKeepsItsName)
{
    Collector::set_thread_name("decoder");
    DrainBatch batch;
    Collector::instance().drain(batch);

    const ThreadDrain* const mine = find(batch, Collector::thread_index());
    ASSERT_NE(mine, nullptr);
    EXPECT_EQ(mine->name, "decoder");
}

TEST_F(Small_Collector, CalibrationPointsAdvance)
{
    DrainBatch first;
    Collector::instance().drain(first);
    DrainBatch second;
    Collector::instance().drain(second);

    EXPECT_LE(Collector::instance().first_point().steady_ns, first.point.steady_ns);
    EXPECT_LE(first.point.steady_ns, second.point.steady_ns);
    EXPECT_LE(first.point.ticks, second.point.ticks);
}

TEST_F(Medium_Collector, EachThreadHasItsOwnRing)
{
    std::vector<std::uint32_t> indices(4U, 0U);
    std::vector<std::thread> threads;
    for (std::size_t slot = 0U; slot < indices.size(); ++slot)
        threads.emplace_back([&, slot] {
            push_events(1000U);
            indices[slot] = Collector::thread_index();
        });
    for (std::thread& thread : threads)
        thread.join();
    DrainBatch batch;
    Collector::instance().drain(batch);

    for (const std::uint32_t index : indices)
    {
        const ThreadDrain* const writer = find(batch, index);
        ASSERT_NE(writer, nullptr);
        EXPECT_EQ(writer->events.size(), 1000U);
    }
}

TEST_F(Medium_Collector, AnExitedThreadIsDrainedThenFreed)
{
    std::uint32_t index = 0U;
    std::thread([&] {
        push_events(10U);
        index = Collector::thread_index();
    }).join();
    DrainBatch first;
    Collector::instance().drain(first);
    DrainBatch second;
    Collector::instance().drain(second);

    const ThreadDrain* const exited = find(first, index);
    ASSERT_NE(exited, nullptr);
    EXPECT_EQ(exited->events.size(), 10U);
    EXPECT_TRUE(exited->retired);
    EXPECT_EQ(find(second, index), nullptr);
}
