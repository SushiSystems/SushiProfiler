/**************************************************************************/
/* test_pause_state.cpp                                                   */
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
 * @file test_pause_state.cpp
 * @brief Tests when a pause or a resume takes effect on a session's pause state.
 * @author Mustafa Garip
 * @see tests/session/session/README.md
 */

#include <SushiProfiler/session/pause_state.hpp>

#include <gtest/gtest.h>

using SushiProfiler::PauseState;

TEST(Small_PauseState, PausesARunningSessionOnce)
{
    PauseState state;

    EXPECT_TRUE(state.pause(true));
    EXPECT_FALSE(state.pause(true));
}

TEST(Small_PauseState, RefusesToPauseASessionThatDoesNotRun)
{
    PauseState state;

    EXPECT_FALSE(state.pause(false));
    EXPECT_FALSE(state.paused());
}

TEST(Small_PauseState, ResumesAPausedSessionOnce)
{
    PauseState state;
    ASSERT_TRUE(state.pause(true));

    EXPECT_TRUE(state.resume(true));
    EXPECT_FALSE(state.resume(true));
}

TEST(Small_PauseState, RefusesToResumeASessionThatWasNotPaused)
{
    PauseState state;

    EXPECT_FALSE(state.resume(true));
}

TEST(Small_PauseState, RefusesToResumeASessionThatDoesNotRun)
{
    PauseState state;
    ASSERT_TRUE(state.pause(true));

    EXPECT_FALSE(state.resume(false));
    EXPECT_TRUE(state.paused());
}

TEST(Small_PauseState, ForgetsThePauseOnClear)
{
    PauseState state;
    ASSERT_TRUE(state.pause(true));

    state.clear();

    EXPECT_FALSE(state.paused());
    EXPECT_TRUE(state.pause(true));
}
