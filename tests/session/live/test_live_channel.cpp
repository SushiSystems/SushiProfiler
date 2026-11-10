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
 * @file test_live_channel.cpp
 * @brief Tests the channel's latest pointer and history ring, and readers racing a publisher.
 * @author Mustafa Garip
 * @see tests/session/live/README.md
 */

#include <SushiProfiler/live/live_channel.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <functional>
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

    /// @brief Publishes the sequences from @p first up to, not including, @p last.
    void publish_range(LiveChannel& channel, std::uint64_t first, std::uint64_t last)
    {
        for (std::uint64_t sequence = first; sequence < last; ++sequence)
            channel.publish(period_of(sequence));
    }

    /// @brief Returns whether @p history holds @p period.
    bool holds(const std::vector<std::shared_ptr<const ClosedPeriod>>& history,
               const ClosedPeriod* period)
    {
        return std::any_of(history.begin(), history.end(),
                           [period](const auto& entry) { return entry.get() == period; });
    }

    /// @brief Reads @p channel until @p done is set and counts in @p violations each time its
    /// history is not consecutive or its newest sequence goes backwards.
    void read_until_done(const LiveChannel& channel, const std::atomic<bool>& done,
                         std::atomic<std::uint64_t>& violations)
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
    publish_range(channel, 0U, 5U);

    const auto last_three = channel.history(3U);

    ASSERT_EQ(last_three.size(), 3U);
    EXPECT_EQ(last_three[0]->sequence, 2U);
    EXPECT_EQ(last_three[2]->sequence, 4U);
    EXPECT_EQ(channel.history(100U).size(), 5U);
}

TEST(Small_LiveChannel, LatestIsTheNewestPublication)
{
    LiveChannel channel(8U);

    publish_range(channel, 0U, 5U);

    EXPECT_EQ(channel.latest()->sequence, 4U);
}

TEST(Small_LiveChannel, PublishedCountsEveryPublication)
{
    LiveChannel channel(8U);

    publish_range(channel, 0U, 5U);

    EXPECT_EQ(channel.published(), 5U);
}

TEST(Small_LiveChannel, TheRingOverwritesItsOldestWhenFull)
{
    LiveChannel channel(3U);
    publish_range(channel, 0U, 7U);

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
    publish_range(channel, 1U, 12U);

    EXPECT_FALSE(holds(channel.history(10U), held.get()));
    EXPECT_EQ(held->sequence, 0U);
    EXPECT_EQ(held->names->name(1U), "name");
}

TEST(Small_LiveChannel, AnOverwrittenPeriodNobodyHoldsIsFreed)
{
    LiveChannel channel(3U);
    channel.publish(period_of(0U));
    const std::weak_ptr<const ClosedPeriod> unheld = channel.latest();
    publish_range(channel, 1U, 12U);

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

TEST(Small_LiveChannel, BeginRunEmptiesTheHistoryAndTheLatest)
{
    LiveChannel channel(4U);
    publish_range(channel, 0U, 2U);

    static_cast<void>(channel.begin_run());

    EXPECT_EQ(channel.latest(), nullptr);
    EXPECT_TRUE(channel.history(10U).empty());
    EXPECT_EQ(channel.published(), 0U);
}

TEST(Small_LiveChannel, BeginRunNumbersTheRun)
{
    LiveChannel channel(4U);
    ASSERT_EQ(channel.run(), 0U);

    EXPECT_EQ(channel.begin_run(), 1U);

    EXPECT_EQ(channel.run(), 1U);
}

TEST(Small_LiveChannel, APeriodHeldAcrossBeginRunSurvivesIt)
{
    LiveChannel channel(4U);
    publish_range(channel, 0U, 2U);
    const auto held = channel.latest();

    static_cast<void>(channel.begin_run());
    channel.publish(period_of(0U));

    EXPECT_EQ(held->sequence, 1U);
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
        readers.emplace_back(read_until_done, std::cref(channel), std::cref(done), std::ref(violations));

    publish_range(channel, 0U, K_PUBLISHES);
    done.store(true, std::memory_order_release);
    for (std::thread& reader : readers)
        reader.join();

    EXPECT_EQ(violations.load(), 0U);
    EXPECT_EQ(channel.latest()->sequence, K_PUBLISHES - 1U);
}
