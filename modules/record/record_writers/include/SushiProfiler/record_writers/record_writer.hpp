/**************************************************************************/
/* record_writer.hpp                                                      */
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
 * @file record_writer.hpp
 * @brief Declares the interface that sends a record and its analyses somewhere.
 * @author Mustafa Garip
 * @see modules/record/record_writers/README.md
 */

#pragma once

#include <SushiProfiler/analysis/table.hpp>
#include <SushiProfiler/record/run_record.hpp>

#include <cstdint>
#include <vector>

namespace SushiProfiler
{
    /// @brief How a write ended.
    enum class WriteStatus : std::uint8_t
    {
        Written,        /**< Everything was written. */
        PathUnwritable, /**< The file could not be opened for writing. */
        StreamFailed    /**< The stream or file failed while being written. */
    };

    /// @brief Writes a record, the tables analysed from it, or both.
    class RecordWriter
    {
        public:
            virtual ~RecordWriter() = default;

            /// @brief Writes what this writer keeps of @p record and @p tables.
            [[nodiscard]] virtual WriteStatus write(const RunRecord& record,
                                                    const std::vector<Table>& tables) = 0;

        protected:
            RecordWriter() = default;
            RecordWriter(const RecordWriter&) = default;
            RecordWriter& operator=(const RecordWriter&) = default;
            RecordWriter(RecordWriter&&) = default;
            RecordWriter& operator=(RecordWriter&&) = default;
    };
} // namespace SushiProfiler
