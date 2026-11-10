/**************************************************************************/
/* chrome_trace_writer.hpp                                                */
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
 * @file chrome_trace_writer.hpp
 * @brief Declares the writer of a Chrome trace that Perfetto and chrome://tracing open.
 * @author Mustafa Garip
 * @see modules/session/stream_writers/README.md
 */

#pragma once

#include <SushiProfiler/stream_writers/chrome_trace_events.hpp>
#include <SushiProfiler/stream_writers/stream_writer.hpp>
#include <SushiProfiler/version/export.hpp>

#include <fstream>
#include <string>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Streams a run as Chrome trace JSON: one event array, written period by period.
     *
     * ChromeTraceEvents builds the events, with the rows and times the trace holds; this writer
     * puts them in the file.
     */
    class SUSHIPROFILER_API ChromeTraceWriter final : public StreamWriter
    {
        public:
            /// @brief Builds a writer of the file at @p path.
            explicit ChromeTraceWriter(std::string path);

            /// @brief Opens the file and writes the process names.
            [[nodiscard]] WriteStatus begin(const RunRecord& record) override;

            /// @brief Writes the period, its zone runs, samples and spans.
            [[nodiscard]] WriteStatus write_period(
                const RunRecord& record, const PeriodRecord& period,
                const std::vector<ZoneRun>& instances) override;

            /// @brief Closes the event array and the file.
            [[nodiscard]] WriteStatus end(const RunRecord& record) override;

        private:
            /// @brief Writes each of @p events after a separator.
            void emit_all(const std::vector<std::string>& events);

            /// @brief Returns the status the file's state gives.
            [[nodiscard]] WriteStatus status() const;

            /// @brief Where the trace goes.
            std::string path_;

            /// @brief The open trace.
            std::ofstream file_;

            /// @brief Whether no event has been written yet.
            bool first_ = true;

            /// @brief Builds the events the file receives.
            ChromeTraceEvents events_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
