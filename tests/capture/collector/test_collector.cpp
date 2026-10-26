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

#include "second_image.hpp"

#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/zones/macros.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using SushiProfiler::DrainBatch;
using SushiProfiler::Collector;
using SushiProfiler::CollectorSettings;
using SushiProfiler::EventKind;
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

TEST_F(Small_Collector, PauseClosesTheGateAndKeepsTheRings)
{
    push_events(4U);
    Collector::instance().pause();

    EXPECT_FALSE(Collector::recording());
    EXPECT_TRUE(Collector::instance().is_open());
    DrainBatch batch;
    Collector::instance().drain(batch);
    const ThreadDrain* const mine = find(batch, Collector::thread_index());
    ASSERT_NE(mine, nullptr);
    EXPECT_EQ(mine->events.size(), 4U);
}

TEST_F(Small_Collector, ResumeOpensTheGateOfAnOpenSession)
{
    Collector::instance().pause();
    Collector::instance().resume();

    EXPECT_TRUE(Collector::recording());
}

TEST_F(Small_Collector, ResumeDoesNothingWhileClosed)
{
    Collector::instance().close();
    Collector::instance().resume();

    EXPECT_FALSE(Collector::recording());
    EXPECT_FALSE(Collector::instance().is_open());
}

namespace
{
    /// @brief Opens the collector for each test and closes and empties it after.
    class Medium_SecondImage : public ::testing::Test
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

            /// @brief Closes the collector and discards what its rings hold.
            static void close_and_empty()
            {
                Collector::instance().close();
                DrainBatch discarded;
                Collector::instance().drain(discarded);
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

TEST_F(Small_Collector, HoldClosesTheZoneGateButKeepsMarking)
{
    Collector::instance().hold();

    EXPECT_TRUE(Collector::instance().is_held());
    EXPECT_FALSE(Collector::recording());
    EXPECT_TRUE(Collector::marking());
}

TEST_F(Small_Collector, ReleaseReopensTheGateOnlyWhereTheWindowWants)
{
    Collector& collector = Collector::instance();
    collector.pause();
    collector.hold();
    collector.release();
    EXPECT_FALSE(Collector::recording());
    EXPECT_FALSE(Collector::marking());
    collector.resume();
    EXPECT_TRUE(Collector::recording());

    collector.hold();
    collector.pause();
    collector.resume();
    EXPECT_FALSE(Collector::recording());
    EXPECT_TRUE(Collector::marking());
    collector.release();
    EXPECT_TRUE(Collector::recording());
}

TEST_F(Small_Collector, CloseClearsAHold)
{
    Collector& collector = Collector::instance();
    collector.hold();
    collector.close();
    ASSERT_TRUE(collector.open(CollectorSettings{}));

    EXPECT_TRUE(Collector::recording());
    EXPECT_FALSE(collector.is_held());
}

TEST_F(Small_Collector, HoldDoesNothingWhileClosed)
{
    Collector& collector = Collector::instance();
    collector.close();
    collector.hold();

    EXPECT_FALSE(collector.is_held());
    EXPECT_FALSE(Collector::marking());
}

TEST_F(Small_Collector, SourcesWantedFollowsWantSources)
{
    EXPECT_FALSE(Collector::sources_wanted());
    Collector::want_sources(true);
    EXPECT_TRUE(Collector::sources_wanted());
    Collector::want_sources(false);
    EXPECT_FALSE(Collector::sources_wanted());
}
