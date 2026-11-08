/**************************************************************************/
/* memory_stream_writer.hpp                                               */
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
 * @file memory_stream_writer.hpp
 * @brief Declares the in-memory stream writer, which keeps every period it is handed.
 * @author Mustafa Garip
 * @see modules/session/stream_writers/README.md
 */

#pragma once

#include <SushiProfiler/record/period_record.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/record_writers/record_writer.hpp>
#include <SushiProfiler/stream_writers/stream_writer.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Keeps a copy of each period and its zone runs, in the order written.
     *
     * Every call returns the status fail_with() set, Written until then; a period whose write
     * does not return Written is not kept. Read it once the session that writes it has stopped.
     */
    class SUSHIPROFILER_API MemoryStreamWriter final : public StreamWriter
    {
        public:
            /// @brief Makes every later call return @p status.
            void fail_with(WriteStatus status) noexcept;

            /// @brief Counts the begin.
            [[nodiscard]] WriteStatus begin(const RunRecord& record) override;

            /// @brief Keeps @p period and @p instances when the write succeeds.
            [[nodiscard]] WriteStatus write_period(const RunRecord& record,
                                                   const PeriodRecord& period,
                                                   const std::vector<ZoneRun>& instances) override;

            /// @brief Counts the end.
            [[nodiscard]] WriteStatus end(const RunRecord& record) override;

            /// @brief Returns how many times begin() ran.
            [[nodiscard]] std::size_t begins() const noexcept;

            /// @brief Returns how many times end() ran.
            [[nodiscard]] std::size_t ends() const noexcept;

            /// @brief Returns how many times write_period() ran, kept or not.
            [[nodiscard]] std::size_t writes() const noexcept;

            /// @brief Returns the periods kept, in the order written.
            [[nodiscard]] const std::vector<PeriodRecord>& periods() const noexcept;

            /// @brief Returns the zone runs of each kept period, beside periods().
            [[nodiscard]] const std::vector<std::vector<ZoneRun>>& instances() const noexcept;

        private:
            /// @brief The status every call returns.
            WriteStatus status_ = WriteStatus::Written;

            /// @brief How many times begin() ran.
            std::size_t begins_ = 0U;

            /// @brief How many times end() ran.
            std::size_t ends_ = 0U;

            /// @brief How many times write_period() ran.
            std::size_t writes_ = 0U;

            /// @brief The kept periods.
            std::vector<PeriodRecord> periods_;

            /// @brief The zone runs of each kept period.
            std::vector<std::vector<ZoneRun>> instances_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
