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
 * @file test_collector.cpp
 * @brief Tests the collector's gate, its thread rings, their retirement and the drain.
 * @author Mustafa Garip
 * @see tests/capture/collector/README.md
 */

#include "second_image.hpp"

#include <SushiProfiler/collector/collector.hpp>

#include <SushiProfiler/clock/manual_tick_source.hpp>
#include <SushiProfiler/zones/macros.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using SushiProfiler::DrainBatch;
using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::ManualTickSource;
using SushiProfiler::EventKind;
using SushiProfiler::ThreadDrain;
using SushiProfiler::ThreadRing;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief Pushes @p count events numbered from 0 on the calling thread's ring of @p collector.
    void push_events(Collector& collector, std::uint64_t count)
    {
        ThreadRing* const ring = collector.thread_ring();
        ASSERT_NE(ring, nullptr);
        for (std::uint64_t value = 0U; value < count; ++value)
        {
            TraceEvent event{};
            event.begin = value;
            ring->push(event);
        }
    }

    /// @brief Returns the drain of thread @p index in @p batch, or null.
    const ThreadDrain* find(const DrainBatch& batch, std::uint32_t index)
    {
        for (const ThreadDrain& thread : batch.threads)
        {
            if (thread.thread_index == index)
                return &thread;
        }
        return nullptr;
    }

    /**
     * @brief Runs @p thread_count threads that each push @p count events into @p collector, drains it
     *        and returns how many events each thread's ring held, in thread order.
     */
    std::vector<std::size_t> events_drained_per_thread(Collector& collector, std::size_t thread_count,
                                                       std::uint64_t count)
    {
        std::vector<std::uint32_t> indices(thread_count, Collector::K_NO_THREAD);
        std::vector<std::thread> threads;
        for (std::size_t slot = 0U; slot < thread_count; ++slot)
        {
            threads.emplace_back(
                [&collector, &indices, slot, count]
                {
                    push_events(collector, count);
                    indices[slot] = collector.index_of_thread();
                });
        }
        for (std::thread& thread : threads)
            thread.join();
        DrainBatch batch;
        collector.drain(batch);

        std::vector<std::size_t> sizes;
        for (const std::uint32_t index : indices)
        {
            const ThreadDrain* const writer = find(batch, index);
            sizes.push_back(writer != nullptr ? writer->events.size() : 0U);
        }
        return sizes;
    }

    /// @brief Gives each test its own open collector on a manual clock that starts at 100 ns.
    class OwnedCollectorTest : public ::testing::Test
    {
        protected:
            void SetUp() override
            {
                ASSERT_TRUE(collector.open(CollectorSettings{}));
            }

            ManualTickSource clock{100U};
            Collector collector{clock};
    };

    /// @brief The process-wide collector, opened for each test and closed and emptied after.
    class ProcessWideCollectorTest : public ::testing::Test
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
    };

    /// @brief The collector's tests that run on one thread and read no process-wide state.
    class Small_Collector : public OwnedCollectorTest
    {
    };

    /// @brief The collector's tests that start threads.
    class Medium_Collector : public ProcessWideCollectorTest
    {
    };

    /// @brief Tests that a second shared image reaches the collector of the executable.
    class Medium_SecondImage : public ProcessWideCollectorTest
    {
        protected:
            /// @brief Returns the names of the zones in @p events, oldest first.
            static std::vector<std::string> zone_names(const std::vector<TraceEvent>& events)
            {
                std::vector<std::string> names;
                for (const TraceEvent& event : events)
                {
                    if (SushiProfiler::kind_of(event) != EventKind::Zone)
                        continue;
                    const SushiProfiler::SiteID site = SushiProfiler::site_of(event);
                    names.push_back(Collector::instance().sites().info(site).name);
                }
                return names;
            }
    };
} // namespace

TEST_F(Small_Collector, OpenOpensTheGate)
{
    EXPECT_TRUE(collector.is_open());
    EXPECT_TRUE(collector.is_recording());
}

TEST_F(Small_Collector, ASecondOpenIsRefused)
{
    EXPECT_FALSE(collector.open(CollectorSettings{}));
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
    push_events(collector, 5U);
    DrainBatch batch;
    collector.drain(batch);

    const ThreadDrain* const mine = find(batch, collector.index_of_thread());
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
        push_events(Collector::instance(), 70U);
        index = Collector::thread_index();
    }).join();
    DrainBatch batch;
    Collector::instance().drain(batch);

    const ThreadDrain* const writer = find(batch, index);
    ASSERT_NE(writer, nullptr);
    EXPECT_EQ(writer->events.size(), 64U);
    EXPECT_EQ(writer->dropped, 6U);
}

TEST(Small_OwnedCollector, OpenDiscardsEventsLeftInTheRingsFromBefore)
{
    const ManualTickSource clock;
    Collector collector(clock);
    ASSERT_TRUE(collector.open(CollectorSettings{}));
    ThreadRing* const ring = collector.thread_ring();
    ASSERT_NE(ring, nullptr);
    static_cast<void>(ring->push(TraceEvent{}));
    collector.close();

    ASSERT_TRUE(collector.open(CollectorSettings{}));

    DrainBatch batch;
    collector.drain(batch);
    ASSERT_EQ(batch.threads.size(), 1U);
    EXPECT_TRUE(batch.threads[0].events.empty());
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
    collector.name_thread("decoder");
    DrainBatch batch;
    collector.drain(batch);

    const ThreadDrain* const mine = find(batch, collector.index_of_thread());
    ASSERT_NE(mine, nullptr);
    EXPECT_EQ(mine->name, "decoder");
}

TEST_F(Small_Collector, CalibrationPointsAdvance)
{
    clock.advance(50U);
    DrainBatch first;
    collector.drain(first);
    clock.advance(25U);
    DrainBatch second;
    collector.drain(second);

    EXPECT_EQ(collector.first_point().steady_ns, 100U);
    EXPECT_EQ(first.point.steady_ns, 150U);
    EXPECT_EQ(second.point.steady_ns, 175U);
    EXPECT_EQ(second.point.ticks, 175U);
}

TEST_F(Medium_Collector, EachThreadHasItsOwnRing)
{
    const std::vector<std::size_t> sizes = events_drained_per_thread(Collector::instance(), 4U, 1000U);

    EXPECT_EQ(sizes, std::vector<std::size_t>(4U, 1000U));
}

TEST_F(Medium_Collector, AnExitedThreadIsDrainedThenFreed)
{
    std::uint32_t index = 0U;
    std::thread([&] {
        push_events(Collector::instance(), 10U);
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

TEST_F(Small_Collector, PauseClosesTheGate)
{
    collector.pause();

    EXPECT_FALSE(collector.is_recording());
    EXPECT_TRUE(collector.is_open());
}

TEST_F(Small_Collector, PauseKeepsTheRings)
{
    push_events(collector, 4U);
    collector.pause();

    DrainBatch batch;
    collector.drain(batch);
    const ThreadDrain* const mine = find(batch, collector.index_of_thread());
    ASSERT_NE(mine, nullptr);
    EXPECT_EQ(mine->events.size(), 4U);
}

TEST_F(Small_Collector, ResumeOpensTheGateOfAnOpenSession)
{
    collector.pause();
    collector.resume();

    EXPECT_TRUE(collector.is_recording());
}

TEST_F(Small_Collector, ResumeDoesNothingWhileClosed)
{
    collector.close();
    collector.resume();

    EXPECT_FALSE(collector.is_recording());
    EXPECT_FALSE(collector.is_open());
}

TEST_F(Medium_SecondImage, AZoneOfTheSecondImageReachesTheCollectorOfTheExecutable)
{
    {
        SP_ZONE("executable_zone");
    }
    SushiProfilerTests::record_zone_in_second_image();

    DrainBatch batch;
    Collector::instance().drain(batch);
    const ThreadDrain* const mine = find(batch, Collector::thread_index());

    ASSERT_NE(mine, nullptr);
    const std::vector<std::string> names = zone_names(mine->events);
    ASSERT_EQ(names.size(), 2U);
    EXPECT_EQ(names[0], "executable_zone");
    EXPECT_EQ(names[1], "second_image_zone");
}

TEST_F(Medium_SecondImage, AThreadThatOnlyRunsTheSecondImageGetsItsRingFromTheSameCollector)
{
    std::uint32_t thread_index = Collector::K_NO_THREAD;
    std::thread(
        [&thread_index]
        {
            SushiProfilerTests::record_zone_in_second_image();
            thread_index = Collector::thread_index();
        })
        .join();

    DrainBatch batch;
    Collector::instance().drain(batch);
    const ThreadDrain* const worker = find(batch, thread_index);

    ASSERT_NE(thread_index, Collector::K_NO_THREAD);
    ASSERT_NE(worker, nullptr);
    EXPECT_NE(thread_index, Collector::thread_index());
    EXPECT_EQ(zone_names(worker->events), std::vector<std::string>{"second_image_zone"});
}

TEST_F(Medium_SecondImage, AClosedCollectorRecordsNothingFromTheSecondImage)
{
    Collector::instance().close();
    SushiProfilerTests::record_zone_in_second_image();

    DrainBatch batch;
    Collector::instance().drain(batch);

    EXPECT_FALSE(Collector::recording());
    const ThreadDrain* const mine = find(batch, Collector::thread_index());
    EXPECT_TRUE(mine == nullptr || mine->events.empty());
}

TEST_F(Small_Collector, HoldClosesTheZoneGate)
{
    collector.hold();

    EXPECT_TRUE(collector.is_held());
    EXPECT_FALSE(collector.is_recording());
}

TEST_F(Small_Collector, HoldKeepsMarking)
{
    collector.hold();

    EXPECT_TRUE(collector.is_marking());
}

TEST_F(Small_Collector, ReleaseKeepsAPausedGateClosed)
{
    collector.pause();
    collector.hold();
    collector.release();

    EXPECT_FALSE(collector.is_recording());
    EXPECT_FALSE(collector.is_marking());
}

TEST_F(Small_Collector, ResumeKeepsAHeldGateClosed)
{
    collector.hold();
    collector.pause();
    collector.resume();

    EXPECT_FALSE(collector.is_recording());
    EXPECT_TRUE(collector.is_marking());
}

TEST_F(Small_Collector, ReleaseOpensTheGateOfAResumedWindow)
{
    collector.hold();
    collector.pause();
    collector.resume();
    collector.release();

    EXPECT_TRUE(collector.is_recording());
}

TEST_F(Small_Collector, CloseClearsAHold)
{
    collector.hold();
    collector.close();
    ASSERT_TRUE(collector.open(CollectorSettings{}));

    EXPECT_TRUE(collector.is_recording());
    EXPECT_FALSE(collector.is_held());
}

TEST_F(Small_Collector, HoldDoesNothingWhileClosed)
{
    collector.close();
    collector.hold();

    EXPECT_FALSE(collector.is_held());
    EXPECT_FALSE(collector.is_marking());
}

TEST_F(Medium_Collector, AThreadWithoutARingHasNoThreadIndex)
{
    std::uint32_t index = 0U;
    std::thread([&index] { index = Collector::thread_index(); }).join();

    EXPECT_EQ(index, Collector::K_NO_THREAD);
}

TEST_F(Medium_Collector, ARetiredThreadGetsNoRing)
{
    ManualTickSource clock{100U};
    Collector collector{clock};
    ASSERT_TRUE(collector.open(CollectorSettings{}));
    ThreadRing* ring = nullptr;
    std::thread(
        [&collector, &ring]
        {
            static_cast<void>(collector.thread_ring());
            Collector::retire_current_thread();
            ring = collector.thread_ring();
        })
        .join();

    EXPECT_EQ(ring, nullptr);
}
