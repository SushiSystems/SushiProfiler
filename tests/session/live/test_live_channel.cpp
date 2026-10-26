/**************************************************************************/
/* test_live_channel.cpp                                                  */
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
 * @file test_live_channel.cpp
 * @brief Tests the channel's latest pointer and history ring, and readers racing a publisher.
 * @author Mustafa Garip
 * @see tests/session/live/README.md
 */

#include <SushiProfiler/live/live_channel.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

using SushiProfiler::ClosedPeriod;
using SushiProfiler::LiveChannel;
using SushiProfiler::NameSnapshot;

namespace
{
    /// @brief Returns a closed period of sequence @p sequence that holds a name snapshot.
    std::shared_ptr<const ClosedPeriod> period_of(std::uint64_t sequence)
    {
        auto period = std::make_shared<ClosedPeriod>();
        period->sequence = sequence;
        period->names = std::make_shared<const NameSnapshot>(
            std::vector<std::string>{"", "name"}, std::vector<SushiProfiler::SiteRecord>{},
            std::vector<std::string>{}, std::vector<SushiProfiler::ThreadRecord>{});
        return period;
    }
} // namespace

TEST(Small_LiveChannel, LatestIsNullBeforeAPublication)
{
    LiveChannel channel(4U);

    EXPECT_EQ(channel.latest(), nullptr);
    EXPECT_TRUE(channel.history(10U).empty());
    EXPECT_EQ(channel.published(), 0U);
}

TEST(Small_LiveChannel, HistoryReturnsTheLastNOldestFirst)
{
    LiveChannel channel(8U);
    for (std::uint64_t sequence = 0U; sequence < 5U; ++sequence)
        channel.publish(period_of(sequence));

    const auto last_three = channel.history(3U);

    ASSERT_EQ(last_three.size(), 3U);
    EXPECT_EQ(last_three[0]->sequence, 2U);
    EXPECT_EQ(last_three[2]->sequence, 4U);
    EXPECT_EQ(channel.history(100U).size(), 5U);
    EXPECT_EQ(channel.latest()->sequence, 4U);
    EXPECT_EQ(channel.published(), 5U);
}

TEST(Small_LiveChannel, TheRingOverwritesItsOldestWhenFull)
{
    LiveChannel channel(3U);
    for (std::uint64_t sequence = 0U; sequence < 7U; ++sequence)
        channel.publish(period_of(sequence));

    const auto history = channel.history(10U);

    ASSERT_EQ(history.size(), 3U);
    EXPECT_EQ(history[0]->sequence, 4U);
    EXPECT_EQ(history[1]->sequence, 5U);
    EXPECT_EQ(history[2]->sequence, 6U);
}

TEST(Small_LiveChannel, AReaderKeepsAnOverwrittenPeriodAlive)
{
    LiveChannel channel(3U);
    channel.publish(period_of(0U));
    const std::shared_ptr<const ClosedPeriod> held = channel.latest();
    channel.publish(period_of(1U));
    const std::weak_ptr<const ClosedPeriod> unheld = channel.latest();
    for (std::uint64_t sequence = 2U; sequence < 12U; ++sequence)
        channel.publish(period_of(sequence));

    for (const auto& period : channel.history(10U))
    {
        EXPECT_NE(period, held);
        EXPECT_NE(period.get(), unheld.lock().get());
    }
    EXPECT_EQ(held->sequence, 0U);
    EXPECT_EQ(held->names->name(1U), "name");
    EXPECT_TRUE(unheld.expired());
}

TEST(Small_LiveChannel, WatchedFollowsAttachAndDetach)
{
    LiveChannel channel(2U);

    EXPECT_FALSE(channel.watched());
    channel.attach();
    channel.attach();
    channel.detach();
    EXPECT_TRUE(channel.watched());
    channel.detach();
    EXPECT_FALSE(channel.watched());
}

TEST(Small_LiveChannel, BeginRunClearsTheHistoryAndBumpsTheRun)
{
    LiveChannel channel(4U);
    channel.publish(period_of(0U));
    channel.publish(period_of(1U));
    const auto held = channel.latest();

    EXPECT_EQ(channel.run(), 0U);
    EXPECT_EQ(channel.begin_run(), 1U);

    EXPECT_EQ(channel.run(), 1U);
    EXPECT_EQ(channel.latest(), nullptr);
    EXPECT_TRUE(channel.history(10U).empty());
    EXPECT_EQ(channel.published(), 0U);
    EXPECT_EQ(held->sequence, 1U);
    channel.publish(period_of(0U));
    EXPECT_EQ(channel.history(10U).size(), 1U);
}

TEST(Medium_LiveChannel, ReadersNeverSeeSequenceGoBackwards)
{
    constexpr std::uint64_t K_PUBLISHES = 10'000U;
    LiveChannel channel(64U);
    std::atomic<bool> done{false};
    std::atomic<std::uint64_t> violations{0U};
    std::vector<std::thread> readers;
    for (int reader = 0; reader < 4; ++reader)
    {
        readers.emplace_back(
            [&]
            {
                std::uint64_t last_latest = 0U;
                while (!done.load(std::memory_order_acquire))
                {
                    const auto history = channel.history(50U);
                    for (std::size_t index = 1U; index < history.size(); ++index)
                    {
                        if (history[index]->sequence != history[index - 1U]->sequence + 1U)
                            violations.fetch_add(1U);
                    }
                    const auto latest = channel.latest();
                    if (latest == nullptr)
                        continue;
                    if (latest->sequence < last_latest)
                        violations.fetch_add(1U);
                    last_latest = latest->sequence;
                    if (!history.empty() && latest->sequence < history.back()->sequence)
                        violations.fetch_add(1U);
                }
            });
    }
    for (std::uint64_t sequence = 0U; sequence < K_PUBLISHES; ++sequence)
        channel.publish(period_of(sequence));
    done.store(true, std::memory_order_release);
    for (std::thread& reader : readers)
        reader.join();

    EXPECT_EQ(violations.load(), 0U);
    EXPECT_EQ(channel.latest()->sequence, K_PUBLISHES - 1U);
}
