/**************************************************************************/
/* test_macros.cpp                                                        */
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
 * @file test_macros.cpp
 * @brief Tests that each direct macro writes its kind at the default level.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#include <SushiProfiler/zones/macros.hpp>

#include "allocation_counter.hpp"
#include "recording_fixture.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

using SushiProfiler::Collector;
using SushiProfiler::EventKind;

namespace
{
    /// @brief The macro tests, on one thread.
    class Small_Macros : public SushiProfilerTests::RecordingFixture
    {
    };

    static_assert(SP_PROFILE_LEVEL == 2, "the direct macros compile in levels 1 and 2 by default");
} // namespace

TEST_F(Small_Macros, EachDirectMacroWritesItsKind)
{
    const SushiProfiler::NameID lane = Collector::instance().names().intern("main");
    {
        SP_LANE(lane);
        {
            SP_ZONE("stage");
        }
        {
            SP_ZONE_GROUP("read", "Data");
        }
        {
            SP_ZONE_FINE("inner");
        }
        SP_COUNTER("bytes", "B", Add, 64.0);
        SP_VALUE("loss", "", 0.5);
        SP_INSTANT("tick");
        SP_FLOW_BEGIN("item", 1U);
        SP_FLOW_STEP("item", 1U);
        SP_FLOW_END("item", 1U);
    }
    std::vector<EventKind> kinds;
    for (const auto& event : drain_mine())
        kinds.push_back(SushiProfiler::kind_of(event));

    const std::vector<EventKind> expected = {
        EventKind::LaneBegin, EventKind::Zone,     EventKind::Zone,     EventKind::Zone,
        EventKind::Counter,   EventKind::Value,    EventKind::Instant,  EventKind::FlowBegin,
        EventKind::FlowStep,  EventKind::FlowEnd,  EventKind::LaneEnd};
    EXPECT_EQ(kinds, expected);
}

TEST_F(Small_Macros, TheRecordingThreadNeverAllocates)
{
    const SushiProfiler::NameID lane = Collector::instance().names().intern("allocations");
    const auto record_every_kind = [lane](std::uint64_t item) {
        SP_LANE(lane);
        {
            SP_ZONE("stage");
        }
        SP_COUNTER("bytes", "B", Add, 1.0);
        SP_VALUE("level", "dB", -6.0);
        SP_INSTANT("tick");
        SP_FLOW_BEGIN("item", item);
        SP_FLOW_END("item", item);
    };
    // The first pass registers every site, which allocates once per site.
    record_every_kind(0U);
    const std::uint64_t before = SushiProfilerTests::allocations_on_this_thread();

    for (std::uint64_t item = 1U; item <= 100'000U; ++item)
        record_every_kind(item);

    EXPECT_EQ(SushiProfilerTests::allocations_on_this_thread() - before, 0U);
}
