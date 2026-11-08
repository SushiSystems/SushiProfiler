/**************************************************************************/
/* test_stream_writer.cpp                                                 */
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
 * @file test_stream_writer.cpp
 * @brief Tests the StreamWriter port's promise on the in-memory writer.
 * @author Mustafa Garip
 * @see tests/session/stream_writers/README.md
 */

#include <SushiProfiler/stream_writers/stream_writer.hpp>

#include <SushiProfiler/stream_writers/memory_stream_writer.hpp>

#include <gtest/gtest.h>

using SushiProfiler::MemoryStreamWriter;
using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::StreamWriter;
using SushiProfiler::WriteStatus;
using SushiProfiler::ZoneRun;

TEST(Small_MemoryStreamWriterContract, ReportsABeginTwoPeriodsAndAnEndWritten)
{
    MemoryStreamWriter memory;
    StreamWriter& writer = memory;
    const RunRecord record;
    PeriodRecord first;
    first.index = 3U;
    PeriodRecord second;
    second.index = 4U;

    EXPECT_EQ(writer.begin(record), WriteStatus::Written);
    EXPECT_EQ(writer.write_period(record, first, {ZoneRun{}}), WriteStatus::Written);
    EXPECT_EQ(writer.write_period(record, second, {}), WriteStatus::Written);
    EXPECT_EQ(writer.end(record), WriteStatus::Written);
}
