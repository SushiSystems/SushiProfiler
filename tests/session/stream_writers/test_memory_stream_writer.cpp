/**************************************************************************/
/* test_memory_stream_writer.cpp                                          */
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
 * @file test_memory_stream_writer.cpp
 * @brief Tests that the in-memory stream writer keeps what it was handed and fails when told.
 * @author Mustafa Garip
 * @see tests/session/stream_writers/README.md
 */

#include <SushiProfiler/stream_writers/memory_stream_writer.hpp>

#include <gtest/gtest.h>

using SushiProfiler::MemoryStreamWriter;
using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::WriteStatus;
using SushiProfiler::ZoneRun;

TEST(Small_MemoryStreamWriter, KeepsThePeriodsAndTheirRunsInTheOrderWritten)
{
    MemoryStreamWriter writer;
    const RunRecord record;
    PeriodRecord first;
    first.index = 3U;
    PeriodRecord second;
    second.index = 4U;

    static_cast<void>(writer.write_period(record, first, {ZoneRun{}}));
    static_cast<void>(writer.write_period(record, second, {}));

    ASSERT_EQ(writer.periods().size(), 2U);
    EXPECT_EQ(writer.periods()[0].index, 3U);
    EXPECT_EQ(writer.periods()[1].index, 4U);
    EXPECT_EQ(writer.instances()[0].size(), 1U);
    EXPECT_TRUE(writer.instances()[1].empty());
}

TEST(Small_MemoryStreamWriter, AFailingWriteReturnsItsStatusAndKeepsNothing)
{
    MemoryStreamWriter writer;
    writer.fail_with(WriteStatus::StreamFailed);

    const WriteStatus status = writer.write_period(RunRecord{}, PeriodRecord{}, {});

    EXPECT_EQ(status, WriteStatus::StreamFailed);
    EXPECT_TRUE(writer.periods().empty());
}

TEST(Small_MemoryStreamWriter, CountsAFailingWriteAmongItsWrites)
{
    MemoryStreamWriter writer;
    writer.fail_with(WriteStatus::StreamFailed);

    static_cast<void>(writer.write_period(RunRecord{}, PeriodRecord{}, {}));

    EXPECT_EQ(writer.writes(), 1U);
}

TEST(Small_MemoryStreamWriter, CountsItsBeginsAndEnds)
{
    MemoryStreamWriter writer;
    const RunRecord record;

    static_cast<void>(writer.begin(record));
    static_cast<void>(writer.end(record));
    static_cast<void>(writer.end(record));

    EXPECT_EQ(writer.begins(), 1U);
    EXPECT_EQ(writer.ends(), 2U);
}
