/**************************************************************************/
/* test_pause_log.cpp                                                     */
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
 * @file test_pause_log.cpp
 * @brief Tests the pause log's overlap answers, its bounds and its use from several threads.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/pause_log.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

using SushiProfiler::PauseLog;

namespace
{
    /// @brief Adds @p count closed intervals to @p log, interval i from 10 i to 10 i + 5.
    void add_intervals(PauseLog& log, std::uint64_t count)
    {
        for (std::uint64_t index = 0U; index < count; ++index)
        {
            log.begin(index * 10U);
            log.end(index * 10U + 5U);
        }
    }

    /// @brief Runs 10 000 intervals against two readers and returns how often a reader saw an
    /// overlap with [0, 500), which no interval has.
    int wrong_overlaps_under_concurrent_use(PauseLog& log)
    {
        std::atomic<bool> done{false};
        std::atomic<int> wrong{0};
        std::vector<std::thread> readers;
        for (int reader = 0; reader < 2; ++reader)
        {
            readers.emplace_back(
                [&]
                {
                    while (!done.load(std::memory_order_acquire))
                    {
                        if (log.overlaps(0U, 500U))
                            wrong.fetch_add(1);
                        static_cast<void>(log.ends_after(0U));
                    }
                });
        }
        for (std::uint64_t index = 0U; index < 10'000U; ++index)
        {
            log.begin(1'000U + index * 10U);
            log.end(1'000U + index * 10U + 5U);
        }
        done.store(true, std::memory_order_release);
        for (std::thread& reader : readers)
            reader.join();
        return wrong.load();
    }

    /// @brief The bounds of a period asked about an interval from 100 to 200.
    struct PeriodRow
    {
        std::uint64_t begin;
        std::uint64_t end;
    };
} // namespace

class Small_PauseLogOverlap : public ::testing::TestWithParam<PeriodRow>
{
};

TEST_P(Small_PauseLogOverlap, APeriodSharingTimeWithAnIntervalOverlapsIt)
{
    PauseLog log;
    log.begin(100U);
    log.end(200U);

    EXPECT_TRUE(log.overlaps(GetParam().begin, GetParam().end));
}

INSTANTIATE_TEST_SUITE_P(Small_Periods, Small_PauseLogOverlap,
                         ::testing::Values(PeriodRow{50U, 150U}, PeriodRow{150U, 250U},
                                           PeriodRow{120U, 130U}));

TEST(Small_PauseLog, ABoundaryTouchIsNotAnOverlap)
{
    PauseLog log;
    log.begin(100U);
    log.end(200U);

    EXPECT_FALSE(log.overlaps(0U, 100U));
    EXPECT_FALSE(log.overlaps(200U, 300U));
}

TEST(Small_PauseLog, AnOpenIntervalOverlapsEverythingAfterItsStart)
{
    PauseLog log;
    log.begin(100U);

    EXPECT_TRUE(log.paused());
    EXPECT_TRUE(log.overlaps(5'000U, 6'000U));
    EXPECT_FALSE(log.overlaps(0U, 100U));
}

TEST(Small_PauseLog, AnEndWithoutABeginIsIgnored)
{
    PauseLog log;

    log.end(10U);

    EXPECT_FALSE(log.paused());
    EXPECT_TRUE(log.ends_after(0U).empty());
}

TEST(Small_PauseLog, ASecondBeginWhileOpenIsIgnored)
{
    PauseLog log;
    log.begin(100U);
    log.begin(150U);
    log.end(200U);

    EXPECT_FALSE(log.paused());
    EXPECT_TRUE(log.overlaps(110U, 120U));
    EXPECT_EQ(log.ends_after(0U).size(), 1U);
}

TEST(Small_PauseLog, ForgetsClosedIntervalsBeforeATime)
{
    PauseLog log;
    log.begin(10U);
    log.end(20U);
    log.begin(100U);
    log.end(200U);

    log.forget_before(50U);

    EXPECT_FALSE(log.overlaps(10U, 20U));
    EXPECT_TRUE(log.overlaps(100U, 200U));
}

TEST(Small_PauseLog, ForgettingKeepsAnOpenInterval)
{
    PauseLog log;
    log.begin(10U);

    log.forget_before(50U);

    EXPECT_TRUE(log.paused());
    EXPECT_TRUE(log.overlaps(60U, 70U));
}

TEST(Small_PauseLog, EndsAfterReturnsOnlyLaterEnds)
{
    PauseLog log;
    log.begin(10U);
    log.end(20U);
    log.begin(100U);
    log.end(200U);

    const std::vector<std::uint64_t> later = log.ends_after(20U);

    ASSERT_EQ(later.size(), 1U);
    EXPECT_EQ(later[0], 200U);
}

TEST(Small_PauseLog, DropsTheOldestIntervalPastTheCap)
{
    PauseLog log;
    add_intervals(log, PauseLog::K_MAX_INTERVALS + 5U);

    EXPECT_EQ(log.ends_after(0U).size(), PauseLog::K_MAX_INTERVALS);
    EXPECT_FALSE(log.overlaps(0U, 5U));
}

TEST(Medium_PauseLog, ConcurrentBeginEndAndOverlapsAgree)
{
    PauseLog log;

    EXPECT_EQ(wrong_overlaps_under_concurrent_use(log), 0);
    EXPECT_FALSE(log.paused());
}
