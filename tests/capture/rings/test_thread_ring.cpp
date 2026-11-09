/**************************************************************************/
/* test_thread_ring.cpp                                                   */
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
 * @file test_thread_ring.cpp
 * @brief Tests the single-producer ring: order, capacity, drops, fill and the thread name.
 * @author Mustafa Garip
 * @see tests/capture/rings/README.md
 */

#include <SushiProfiler/rings/thread_ring.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

using SushiProfiler::ThreadRing;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief Returns an event whose begin field is @p value.
    TraceEvent event_numbered(std::uint64_t value)
    {
        TraceEvent event{};
        event.begin = value;
        return event;
    }

    /// @brief Pushes events numbered from 0 up to @p count into @p ring; returns how many it kept.
    std::uint64_t push_numbered(ThreadRing& ring, std::uint64_t count)
    {
        std::uint64_t kept = 0U;
        for (std::uint64_t value = 0U; value < count; ++value)
            kept += ring.push(event_numbered(value)) ? 1U : 0U;
        return kept;
    }

    /// @brief Returns the numbers 0 up to @p count, in order.
    std::vector<std::uint64_t> numbers_below(std::uint64_t count)
    {
        std::vector<std::uint64_t> numbers(count);
        std::iota(numbers.begin(), numbers.end(), std::uint64_t{0U});
        return numbers;
    }

    /// @brief Returns the begin field of each of @p events, in order.
    std::vector<std::uint64_t> begins_of(const std::vector<TraceEvent>& events)
    {
        std::vector<std::uint64_t> begins;
        begins.reserve(events.size());
        std::transform(events.begin(), events.end(), std::back_inserter(begins),
                       [](const TraceEvent& event) { return event.begin; });
        return begins;
    }

    /// @brief Drains @p ring into @p out until @p producing turns false, then once more.
    void drain_while_producing(ThreadRing& ring, const std::atomic<bool>& producing,
                               std::vector<TraceEvent>& out)
    {
        while (producing.load(std::memory_order_acquire))
            ring.drain(out);
        ring.drain(out);
    }

    /// @brief Returns whether each begin field of @p events is larger than the one before it.
    bool strictly_increasing(const std::vector<TraceEvent>& events)
    {
        return std::adjacent_find(events.begin(), events.end(),
                                  [](const TraceEvent& before, const TraceEvent& after)
                                  { return before.begin >= after.begin; }) == events.end();
    }
} // namespace

TEST(Small_ThreadRing, CapacityRoundsUpToAPowerOfTwo)
{
    EXPECT_EQ(ThreadRing(100U).capacity(), 128U);
    EXPECT_EQ(ThreadRing(1U).capacity(), 64U);
    EXPECT_EQ(ThreadRing(4096U).capacity(), 4096U);
}

TEST(Small_ThreadRing, DrainsInOrder)
{
    ThreadRing ring(64U);
    push_numbered(ring, 10U);
    std::vector<TraceEvent> drained;

    EXPECT_EQ(ring.drain(drained), 10U);
    EXPECT_EQ(begins_of(drained), numbers_below(10U));
}

TEST(Small_ThreadRing, ADrainedRingDrainsNothingMore)
{
    ThreadRing ring(64U);
    push_numbered(ring, 10U);
    std::vector<TraceEvent> drained;
    ring.drain(drained);

    EXPECT_EQ(ring.drain(drained), 0U);
}

TEST(Small_ThreadRing, AFullRingKeepsItsCapacity)
{
    ThreadRing ring(64U);

    EXPECT_EQ(push_numbered(ring, 70U), 64U);
}

TEST(Small_ThreadRing, AFullRingCountsWhatItDropped)
{
    ThreadRing ring(64U);
    push_numbered(ring, 70U);

    EXPECT_EQ(ring.take_dropped(), 6U);
    EXPECT_EQ(ring.take_dropped(), 0U);
}

TEST(Small_ThreadRing, FillFollowsOccupancy)
{
    ThreadRing ring(64U);
    push_numbered(ring, 32U);

    EXPECT_DOUBLE_EQ(ring.fill(), 0.5);
}

TEST(Small_ThreadRing, FillIsZeroOnceDrained)
{
    ThreadRing ring(64U);
    push_numbered(ring, 32U);
    std::vector<TraceEvent> drained;
    ring.drain(drained);

    EXPECT_DOUBLE_EQ(ring.fill(), 0.0);
}

TEST(Small_ThreadRing, FillIsOneWhenTheRingIsFull)
{
    ThreadRing ring(64U);
    push_numbered(ring, 64U);

    EXPECT_DOUBLE_EQ(ring.fill(), 1.0);
}

TEST(Small_ThreadRing, AnEmptyNameReadsEmpty)
{
    ThreadRing ring(64U);
    ring.set_name("audio");
    ring.set_name("");

    EXPECT_EQ(ring.name(), "");
}

TEST(Small_ThreadRing, NameIsTruncatedAtSixtyThree)
{
    ThreadRing ring(64U);
    ring.set_name(std::string(100U, 'x'));

    EXPECT_EQ(ring.name(), std::string(63U, 'x'));
}

TEST(Small_ThreadRing, ASecondNameReplacesTheFirst)
{
    ThreadRing ring(64U);
    ring.set_name(std::string(100U, 'x'));
    ring.set_name("audio");

    EXPECT_EQ(ring.name(), "audio");
}

TEST(Medium_ThreadRing, OneProducerOneConsumerLoseNothingThatIsNotCounted)
{
    constexpr std::uint64_t K_EVENTS = 1'000'000U;
    ThreadRing ring(4096U);
    std::atomic<bool> producing{true};
    std::vector<TraceEvent> drained;
    drained.reserve(K_EVENTS);
    std::thread consumer(drain_while_producing, std::ref(ring), std::cref(producing),
                         std::ref(drained));
    push_numbered(ring, K_EVENTS);
    producing.store(false, std::memory_order_release);
    consumer.join();

    EXPECT_EQ(drained.size() + ring.take_dropped(), K_EVENTS);
    EXPECT_TRUE(strictly_increasing(drained));
}
