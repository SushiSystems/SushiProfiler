/**************************************************************************/
/* test_record_file_writer.cpp                                            */
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
 * @file test_record_file_writer.cpp
 * @brief Tests that the file writer saves a record that reads back, and names an unwritable path.
 * @author Mustafa Garip
 * @see tests/record/record_writers/README.md
 */

#include <SushiProfiler/record_writers/record_file_writer.hpp>

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include <SushiProfiler/record_json/record_json.hpp>

using SushiProfiler::RecordFileWriter;
using SushiProfiler::RecordReadResult;
using SushiProfiler::RunRecord;
using SushiProfiler::WriteStatus;

TEST(Medium_RecordFileWriter, SavesARecordThatReadsBack)
{
    RunRecord record;
    record.names = {"", "step"};
    record.environment = {{"os", "test"}};
    const std::string path = ::testing::TempDir() + "sushiprofiler_record_file_writer.sprec.json";
    RecordFileWriter writer(path);

    ASSERT_EQ(writer.write(record, {}), WriteStatus::Written);
    std::ifstream file(path, std::ios::binary);
    const std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    file.close();
    std::remove(path.c_str());
    EXPECT_EQ(text, SushiProfiler::write_record(record));
    const RecordReadResult result = SushiProfiler::read_record(text);
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_EQ(*result.record, record);
}

TEST(Medium_RecordFileWriter, NamesAPathItCannotOpen)
{
    RecordFileWriter writer(::testing::TempDir() + "missing_folder/none/record.json");

    EXPECT_EQ(writer.write(RunRecord{}, {}), WriteStatus::PathUnwritable);
}
