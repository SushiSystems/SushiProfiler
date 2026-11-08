/**************************************************************************/
/* test_record_writer.cpp                                                 */
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
 * @file test_record_writer.cpp
 * @brief Tests the RecordWriter port's promise on the in-memory writer and the terminal writer.
 * @author Mustafa Garip
 * @see tests/record/record_writers/README.md
 */

#include <SushiProfiler/record_writers/record_writer.hpp>

#include <gtest/gtest.h>

#include <sstream>
#include <vector>

#include <SushiProfiler/record_writers/memory_record_writer.hpp>
#include <SushiProfiler/record_writers/terminal_writer.hpp>

using SushiProfiler::MemoryRecordWriter;
using SushiProfiler::RecordWriter;
using SushiProfiler::RunRecord;
using SushiProfiler::Table;
using SushiProfiler::TerminalWriter;
using SushiProfiler::WriteStatus;

namespace
{
    /// @brief Expects @p writer to report a record with two tables written.
    void expect_writes_a_record(RecordWriter& writer)
    {
        Table table;
        table.name = "t";

        EXPECT_EQ(writer.write(RunRecord{}, {table, table}), WriteStatus::Written);
    }
} // namespace

TEST(Small_MemoryRecordWriterContract, ReportsARecordWritten)
{
    MemoryRecordWriter writer;

    expect_writes_a_record(writer);
}

TEST(Small_TerminalWriterContract, ReportsARecordWritten)
{
    std::ostringstream out;
    TerminalWriter writer(out);

    expect_writes_a_record(writer);
}
