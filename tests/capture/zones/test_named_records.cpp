/**************************************************************************/
/* test_named_records.cpp                                                 */
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
 * @file test_named_records.cpp
 * @brief Tests the counters and values named at run time, and SP_THREAD_NAME.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#include <SushiProfiler/zones/named_macros.hpp>

#include "recording_fixture.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using SushiProfiler::Collector;
using SushiProfiler::CounterOp;
using SushiProfiler::EventKind;
using SushiProfiler::SiteID;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief The named-record tests, on one thread and a collector of their own.
    class Small_NamedRecords : public SushiProfilerTests::OwnedRecordingFixture
    {
        protected:
            /// @brief Returns the registered name of the site of @p event.
            std::string site_name(const TraceEvent& event)
            {
                const SiteID site = SushiProfiler::site_of(event);
                return collector.sites().info(site).name;
            }
    };

    /// @brief The named-record tests that go through the macros and so reach the process-wide
    ///        collector, or start a thread.
    class Medium_NamedRecords : public SushiProfilerTests::RecordingFixture
    {
    };
} // namespace

TEST_F(Small_NamedRecords, ACounterNamedAtRunTimeGetsOneSitePerName)
{
    SushiProfiler::record_counter_named(collector, "named.counter.0", "items", CounterOp::Add, 1U,
                                        3.0);
    SushiProfiler::record_counter_named(collector, "named.counter.1", "items", CounterOp::Add, 1U,
                                        3.0);
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Counter);
    EXPECT_EQ(site_name(events[0]), "named.counter.0");
    EXPECT_EQ(site_name(events[1]), "named.counter.1");
}

TEST_F(Small_NamedRecords, ACounterNamedAtRunTimeKeepsItsOperationAndUnit)
{
    SushiProfiler::record_counter_named(collector, "named.counter.op", "items", CounterOp::Add, 1U,
                                        3.0);
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 1U);
    const SushiProfiler::SiteInfo info = collector.sites().info(SushiProfiler::site_of(events[0]));
    EXPECT_EQ(info.op, CounterOp::Add);
    EXPECT_EQ(info.unit, "items");
    EXPECT_EQ(info.role, SushiProfiler::SiteRole::Counter);
}

TEST_F(Small_NamedRecords, AValueNamedAtRunTimeKeepsEverySample)
{
    SushiProfiler::record_value_named(collector, "named.value", "dB", 1U, -6.0);
    SushiProfiler::record_value_named(collector, "named.value", "dB", 1U, -3.0);
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(SushiProfiler::kind_of(events[1]), EventKind::Value);
    EXPECT_EQ(SushiProfiler::value_of(events[0].end), -6.0);
    EXPECT_EQ(SushiProfiler::value_of(events[1].end), -3.0);
}

TEST_F(Small_NamedRecords, AValueNamedAtRunTimeKeepsOneSitePerName)
{
    SushiProfiler::record_value_named(collector, "named.value.site", "dB", 1U, -6.0);
    SushiProfiler::record_value_named(collector, "named.value.site", "dB", 1U, -3.0);
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(SushiProfiler::site_of(events[0]), SushiProfiler::site_of(events[1]));
    EXPECT_EQ(site_name(events[0]), "named.value.site");
}

TEST_F(Small_NamedRecords, NamesAreKeptPerRole)
{
    SushiProfiler::record_counter_named(collector, "shared.name", "", CounterOp::Add, 1U, 1.0);
    SushiProfiler::record_value_named(collector, "shared.name", "", 1U, 1.0);
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_NE(SushiProfiler::site_of(events[0]), SushiProfiler::site_of(events[1]));
}

TEST_F(Medium_NamedRecords, ANameIsNotBuiltWhileTheGateIsClosed)
{
    int built = 0;
    const auto build = [&built]
    {
        ++built;
        return std::string("closed.gate.name");
    };
    Collector::instance().hold();
    SP_COUNTER_NAMED(build(), "", Add, 1.0);
    SP_VALUE_NAMED(build(), "", 1.0);

    EXPECT_EQ(built, 0);
    EXPECT_TRUE(drain_mine().empty());
}

TEST_F(Medium_NamedRecords, ThreadNameNamesTheRing)
{
    std::thread(
        []
        {
            SP_THREAD_NAME("render.worker");
            SushiProfiler::DrainBatch batch;
            Collector::instance().drain(batch);
            bool found = false;
            for (const SushiProfiler::ThreadDrain& drain : batch.threads)
            {
                if (drain.thread_index == Collector::thread_index())
                    found = drain.name == "render.worker";
            }
            EXPECT_TRUE(found);
        })
        .join();
}
