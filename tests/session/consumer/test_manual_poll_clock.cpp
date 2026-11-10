/**************************************************************************/
/* test_manual_poll_clock.cpp                                             */
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
 * @file test_manual_poll_clock.cpp
 * @brief Tests that the in-memory poll clock notes the limit of each wait.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/manual_poll_clock.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

using SushiProfiler::ManualPollClock;

TEST(Small_ManualPollClock, NotesTheLimitOfEachWait)
{
    ManualPollClock clock;
    // A wake is one flag, not a count: two wakes before a wait end only that wait.
    clock.wake();
    EXPECT_TRUE(clock.wait(std::chrono::milliseconds(3)));
    clock.wake();
    EXPECT_TRUE(clock.wait(std::chrono::milliseconds(9)));

    const std::vector<std::chrono::milliseconds> expected{std::chrono::milliseconds(3),
                                                          std::chrono::milliseconds(9)};
    EXPECT_EQ(clock.limits(), expected);
}

TEST(Medium_ManualPollClock, PollOnceEndsAWaitAndReturnsOnTheNextOne)
{
    ManualPollClock clock;
    std::thread consumer(
        [&clock]
        {
            static_cast<void>(clock.wait(std::chrono::milliseconds(7)));
            static_cast<void>(clock.wait(std::chrono::milliseconds(9)));
        });

    clock.poll_once();
    clock.stop();
    consumer.join();

    const std::vector<std::chrono::milliseconds> expected{std::chrono::milliseconds(7),
                                                          std::chrono::milliseconds(9)};
    EXPECT_EQ(clock.limits(), expected);
}
