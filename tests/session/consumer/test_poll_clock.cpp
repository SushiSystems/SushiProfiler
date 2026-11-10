/**************************************************************************/
/* test_poll_clock.cpp                                                    */
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
 * @file test_poll_clock.cpp
 * @brief Tests that a wake or a stop ends a poll clock's wait and a rearm undoes a stop.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/poll_clock.hpp>

#include <SushiProfiler/consumer/manual_poll_clock.hpp>
#include <SushiProfiler/consumer/steady_poll_clock.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <vector>

using SushiProfiler::ManualPollClock;
using SushiProfiler::PollClock;
using SushiProfiler::SteadyPollClock;
using std::chrono::hours;

namespace
{
    /// @brief Expects a wake before a wait to end that wait with true, however long its limit.
    void expect_a_wake_ends_the_wait(PollClock& clock)
    {
        clock.wake();

        EXPECT_TRUE(clock.wait(hours(1)));
    }

    /// @brief Expects a stop before a wait to end that wait with false, however long its limit.
    void expect_a_stop_ends_the_wait_with_false(PollClock& clock)
    {
        clock.stop();

        EXPECT_FALSE(clock.wait(hours(1)));
    }

    /// @brief Expects a rearm after a stop to let a woken wait return true again.
    void expect_a_rearm_undoes_a_stop(PollClock& clock)
    {
        clock.stop();
        clock.rearm();
        clock.wake();

        EXPECT_TRUE(clock.wait(hours(1)));
    }
} // namespace

TEST(Small_SteadyPollClockContract, EndsAWaitOnAWake)
{
    SteadyPollClock clock;

    expect_a_wake_ends_the_wait(clock);
}

TEST(Small_SteadyPollClockContract, EndsAWaitWithFalseOnAStop)
{
    SteadyPollClock clock;

    expect_a_stop_ends_the_wait_with_false(clock);
}

TEST(Small_SteadyPollClockContract, WaitsAgainAfterARearm)
{
    SteadyPollClock clock;

    expect_a_rearm_undoes_a_stop(clock);
}

TEST(Small_ManualPollClockContract, EndsAWaitOnAWake)
{
    ManualPollClock clock;

    expect_a_wake_ends_the_wait(clock);
}

TEST(Small_ManualPollClockContract, EndsAWaitWithFalseOnAStop)
{
    ManualPollClock clock;

    expect_a_stop_ends_the_wait_with_false(clock);
}

TEST(Small_ManualPollClockContract, WaitsAgainAfterARearm)
{
    ManualPollClock clock;

    expect_a_rearm_undoes_a_stop(clock);
}
