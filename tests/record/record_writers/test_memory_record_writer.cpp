/**************************************************************************/
/* test_memory_record_writer.cpp                                          */
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
 * @file test_memory_record_writer.cpp
 * @brief Tests that the in-memory record writer keeps what it was handed and fails when told.
 * @author Mustafa Garip
 * @see tests/record/record_writers/README.md
 */

#include <SushiProfiler/record_writers/memory_record_writer.hpp>

#include <gtest/gtest.h>

using SushiProfiler::MemoryRecordWriter;
using SushiProfiler::RunRecord;
using SushiProfiler::Table;
using SushiProfiler::WriteStatus;

TEST(Small_MemoryRecordWriter, KeepsTheRecordAndItsTables)
{
    MemoryRecordWriter writer;
    RunRecord record;
    record.names = {"", "step"};
    Table table;
    table.name = "budget";

    static_cast<void>(writer.write(record, {table}));

    ASSERT_EQ(writer.records().size(), 1U);
    EXPECT_EQ(writer.records()[0].names, record.names);
    ASSERT_EQ(writer.tables()[0].size(), 1U);
    EXPECT_EQ(writer.tables()[0][0].name, "budget");
}

TEST(Small_MemoryRecordWriter, AFailingWriteReturnsItsStatusAndKeepsNothing)
{
    MemoryRecordWriter writer;
    writer.fail_with(WriteStatus::PathUnwritable);

    const WriteStatus status = writer.write(RunRecord{}, {});

    EXPECT_EQ(status, WriteStatus::PathUnwritable);
    EXPECT_TRUE(writer.records().empty());
}
