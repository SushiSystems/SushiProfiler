/**************************************************************************/
/* memory_record_writer.hpp                                               */
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
 * @file memory_record_writer.hpp
 * @brief Declares the in-memory record writer, which keeps every record and table it is handed.
 * @author Mustafa Garip
 * @see modules/record/record_writers/README.md
 */

#pragma once

#include <SushiProfiler/analysis/table.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/record_writers/record_writer.hpp>
#include <SushiProfiler/version/export.hpp>

#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Keeps a copy of each record and its tables, in the order written.
     *
     * write() returns the status fail_with() set, Written until then; a write that does not
     * return Written keeps nothing.
     */
    class SUSHIPROFILER_API MemoryRecordWriter final : public RecordWriter
    {
        public:
            /// @brief Makes every later write return @p status.
            void fail_with(WriteStatus status) noexcept;

            /// @brief Keeps @p record and @p tables when the write succeeds.
            [[nodiscard]] WriteStatus write(const RunRecord& record,
                                            const std::vector<Table>& tables) override;

            /// @brief Returns the records kept, in the order written.
            [[nodiscard]] const std::vector<RunRecord>& records() const noexcept;

            /// @brief Returns the tables of each kept record, beside records().
            [[nodiscard]] const std::vector<std::vector<Table>>& tables() const noexcept;

        private:
            /// @brief The status every write returns.
            WriteStatus status_ = WriteStatus::Written;

            /// @brief The kept records.
            std::vector<RunRecord> records_;

            /// @brief The tables of each kept record.
            std::vector<std::vector<Table>> tables_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
