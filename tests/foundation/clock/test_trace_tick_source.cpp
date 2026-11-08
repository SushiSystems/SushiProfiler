/**************************************************************************/
/* test_trace_tick_source.cpp                                             */
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
 * @file test_trace_tick_source.cpp
 * @brief Tests that the tick source on the processor's counter stamps with ticks when it may.
 * @author Mustafa Garip
 * @see tests/foundation/clock/README.md
 */

#include <SushiProfiler/clock/trace_tick_source.hpp>

#include <SushiProfiler/clock/trace_clock.hpp>

#include <gtest/gtest.h>

using SushiProfiler::TraceClock;
using SushiProfiler::TraceTickSource;

TEST(Small_TraceTickSource, StampsWithTicksExactlyWhenTheCounterIsInvariant)
{
    const TraceTickSource clock;

    EXPECT_EQ(clock.ticks_are_tsc(), TraceClock::ticks_are_invariant());
}
