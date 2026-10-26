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
 * @file test_thread_ring.cpp
 * @brief Tests the single-producer ring: order, capacity, drops, fill and the thread name.
 * @author Mustafa Garip
 * @see tests/capture/rings/README.md
 */

#include <SushiProfiler/rings/thread_ring.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
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
    for (std::uint64_t value = 0U; value < 10U; ++value)
        ASSERT_TRUE(ring.push(event_numbered(value)));
    std::vector<TraceEvent> drained;

    EXPECT_EQ(ring.drain(drained), 10U);
    ASSERT_EQ(drained.size(), 10U);
    for (std::uint64_t value = 0U; value < 10U; ++value)
        EXPECT_EQ(drained[value].begin, value);
    EXPECT_EQ(ring.drain(drained), 0U);
}

TEST(Small_ThreadRing, AFullRingDropsAndCounts)
{
    ThreadRing ring(64U);
    int kept = 0;
    for (std::uint64_t value = 0U; value < 70U; ++value)
        kept += ring.push(event_numbered(value)) ? 1 : 0;
    std::vector<TraceEvent> drained;
    ring.drain(drained);

    EXPECT_EQ(kept, 64);
    EXPECT_EQ(drained.size(), 64U);
    EXPECT_EQ(ring.take_dropped(), 6U);
    EXPECT_EQ(ring.take_dropped(), 0U);
}

TEST(Small_ThreadRing, FillFollowsOccupancy)
{
    ThreadRing ring(64U);
    for (std::uint64_t value = 0U; value < 32U; ++value)
        ring.push(event_numbered(value));

    EXPECT_DOUBLE_EQ(ring.fill(), 0.5);

    std::vector<TraceEvent> drained;
    ring.drain(drained);

    EXPECT_DOUBLE_EQ(ring.fill(), 0.0);
}

TEST(Small_ThreadRing, NameIsTruncatedAtSixtyThree)
{
    ThreadRing ring(64U);
    ring.set_name(std::string(100U, 'x'));

    EXPECT_EQ(ring.name(), std::string(63U, 'x'));

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
    std::thread consumer([&] {
        while (producing.load(std::memory_order_acquire))
            ring.drain(drained);
        ring.drain(drained);
    });
    for (std::uint64_t value = 0U; value < K_EVENTS; ++value)
        ring.push(event_numbered(value));
    producing.store(false, std::memory_order_release);
    consumer.join();

    EXPECT_EQ(drained.size() + ring.take_dropped(), K_EVENTS);
    for (std::size_t index = 1U; index < drained.size(); ++index)
        ASSERT_LT(drained[index - 1U].begin, drained[index].begin);
}
