/**************************************************************************/
/* memory_record_writer.cpp                                               */
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
 * @file memory_record_writer.cpp
 * @brief Keeps the records handed to the in-memory record writer.
 * @author Mustafa Garip
 * @see modules/record/record_writers/README.md
 */

#include <SushiProfiler/record_writers/memory_record_writer.hpp>

namespace SushiProfiler
{
    void MemoryRecordWriter::fail_with(WriteStatus status) noexcept
    {
        status_ = status;
    }

    WriteStatus MemoryRecordWriter::write(const RunRecord& record, const std::vector<Table>& tables)
    {
        if (status_ != WriteStatus::Written)
            return status_;
        records_.push_back(record);
        tables_.push_back(tables);
        return status_;
    }

    const std::vector<RunRecord>& MemoryRecordWriter::records() const noexcept
    {
        return records_;
    }

    const std::vector<std::vector<Table>>& MemoryRecordWriter::tables() const noexcept
    {
        return tables_;
    }
} // namespace SushiProfiler
