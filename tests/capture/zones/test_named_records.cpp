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
    /// @brief The named-record tests, on one thread.
    class Small_NamedRecords : public SushiProfilerTests::RecordingFixture
    {
    };

    /// @brief Returns the registered name of the site of @p event.
    std::string site_name(const TraceEvent& event)
    {
        const SiteID site = SushiProfiler::site_of(event);
        return Collector::instance().sites().info(site).name;
    }
} // namespace

TEST_F(Small_NamedRecords, ACounterNamedAtRunTimeFoldsPerName)
{
    for (int stream = 0; stream < 2; ++stream)
    {
        const std::string name = "named.counter." + std::to_string(stream);
        SP_COUNTER_NAMED(name, "items", Add, 3.0);
    }
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(SushiProfiler::kind_of(events[0]), EventKind::Counter);
    EXPECT_EQ(site_name(events[0]), "named.counter.0");
    EXPECT_EQ(site_name(events[1]), "named.counter.1");
    const SushiProfiler::SiteInfo info =
        Collector::instance().sites().info(SushiProfiler::site_of(events[0]));
    EXPECT_EQ(info.op, CounterOp::Add);
    EXPECT_EQ(info.unit, "items");
    EXPECT_EQ(info.role, SushiProfiler::SiteRole::Counter);
}

TEST_F(Small_NamedRecords, AValueNamedAtRunTimeKeepsEverySample)
{
    const std::string name = "named.value";
    SP_VALUE_NAMED(name, "dB", -6.0);
    SP_VALUE_NAMED(name, "dB", -3.0);
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(SushiProfiler::kind_of(events[1]), EventKind::Value);
    EXPECT_EQ(SushiProfiler::site_of(events[0]), SushiProfiler::site_of(events[1]));
    EXPECT_EQ(site_name(events[0]), "named.value");
}

TEST_F(Small_NamedRecords, ANameIsNotBuiltWhileTheGateIsClosed)
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

TEST_F(Small_NamedRecords, NamesAreKeptPerRole)
{
    const std::string name = "shared.name";
    SP_COUNTER_NAMED(name, "", Add, 1.0);
    SP_VALUE_NAMED(name, "", 1.0);
    const std::vector<TraceEvent> events = drain_mine();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_NE(SushiProfiler::site_of(events[0]), SushiProfiler::site_of(events[1]));
}

TEST_F(Small_NamedRecords, ThreadNameNamesTheRing)
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
