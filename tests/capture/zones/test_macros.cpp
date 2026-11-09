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
using SushiProfiler::NameID;

namespace
{
    /// @brief The macro tests; the macros record into the process-wide collector.
    class Medium_Macros : public SushiProfilerTests::RecordingFixture
    {
        protected:
            /// @brief Runs @p record and returns the kinds of the events it wrote, in order.
            template <class Record>
            static std::vector<EventKind> kinds_written_by(Record record)
            {
                record();
                std::vector<EventKind> kinds;
                for (const auto& event : drain_mine())
                    kinds.push_back(SushiProfiler::kind_of(event));
                return kinds;
            }
    };

    /// @brief Records a zone, a counter, a value, an instant and a flow of @p item in lane @p lane.
    void record_every_kind(NameID lane, std::uint64_t item)
    {
        SP_LANE(lane);
        {
            SP_ZONE("stage");
        }
        SP_COUNTER("bytes", "B", Add, 1.0);
        SP_VALUE("level", "dB", -6.0);
        SP_INSTANT("tick");
        SP_FLOW_BEGIN("item", item);
        SP_FLOW_END("item", item);
    }

    /// @brief Records every kind once for each item from 1 up to @p rounds.
    void record_every_kind_for(NameID lane, std::uint64_t rounds)
    {
        for (std::uint64_t item = 1U; item <= rounds; ++item)
            record_every_kind(lane, item);
    }

    static_assert(SP_PROFILE_LEVEL == 2, "the direct macros compile in levels 1 and 2 by default");
} // namespace

TEST_F(Medium_Macros, SpLaneWritesLaneBeginAndLaneEnd)
{
    const NameID lane = Collector::instance().names().intern("main");

    const std::vector<EventKind> kinds = kinds_written_by([lane] { SP_LANE(lane); });

    EXPECT_EQ(kinds, (std::vector<EventKind>{EventKind::LaneBegin, EventKind::LaneEnd}));
}

TEST_F(Medium_Macros, SpZoneWritesAZone)
{
    EXPECT_EQ(kinds_written_by([] { SP_ZONE("stage"); }),
              std::vector<EventKind>{EventKind::Zone});
}

TEST_F(Medium_Macros, SpZoneGroupWritesAZone)
{
    EXPECT_EQ(kinds_written_by([] { SP_ZONE_GROUP("read", "Data"); }),
              std::vector<EventKind>{EventKind::Zone});
}

TEST_F(Medium_Macros, SpZoneFineWritesAZone)
{
    EXPECT_EQ(kinds_written_by([] { SP_ZONE_FINE("inner"); }),
              std::vector<EventKind>{EventKind::Zone});
}

TEST_F(Medium_Macros, SpCounterWritesACounter)
{
    EXPECT_EQ(kinds_written_by([] { SP_COUNTER("bytes", "B", Add, 64.0); }),
              std::vector<EventKind>{EventKind::Counter});
}

TEST_F(Medium_Macros, SpValueWritesAValue)
{
    EXPECT_EQ(kinds_written_by([] { SP_VALUE("loss", "", 0.5); }),
              std::vector<EventKind>{EventKind::Value});
}

TEST_F(Medium_Macros, SpInstantWritesAnInstant)
{
    EXPECT_EQ(kinds_written_by([] { SP_INSTANT("tick"); }),
              std::vector<EventKind>{EventKind::Instant});
}

TEST_F(Medium_Macros, SpFlowBeginWritesAFlowBegin)
{
    EXPECT_EQ(kinds_written_by([] { SP_FLOW_BEGIN("item", 1U); }),
              std::vector<EventKind>{EventKind::FlowBegin});
}

TEST_F(Medium_Macros, SpFlowStepWritesAFlowStep)
{
    EXPECT_EQ(kinds_written_by([] { SP_FLOW_STEP("item", 1U); }),
              std::vector<EventKind>{EventKind::FlowStep});
}

TEST_F(Medium_Macros, SpFlowEndWritesAFlowEnd)
{
    EXPECT_EQ(kinds_written_by([] { SP_FLOW_END("item", 1U); }),
              std::vector<EventKind>{EventKind::FlowEnd});
}

TEST_F(Medium_Macros, TheRecordingThreadNeverAllocates)
{
    const NameID lane = Collector::instance().names().intern("allocations");
    record_every_kind(lane, 0U);
    const std::uint64_t before = SushiProfilerTests::allocations_on_this_thread();

    record_every_kind_for(lane, 100'000U);

    EXPECT_EQ(SushiProfilerTests::allocations_on_this_thread() - before, 0U);
}
