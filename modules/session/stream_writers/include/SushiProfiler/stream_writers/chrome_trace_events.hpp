/**************************************************************************/
/* chrome_trace_events.hpp                                                */
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
 * @file chrome_trace_events.hpp
 * @brief Declares the builder of Chrome trace events, apart from the file they are written to.
 * @author Mustafa Garip
 * @see modules/session/stream_writers/README.md
 */

#pragma once

#include <SushiProfiler/stream_writers/stream_writer.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Builds the events of a Chrome trace as JSON text, one string per event.
     *
     * Process 1 holds the host: one row per thread, one per lane under it and one per timeline.
     * Process 2 holds one row per device track. Times are microseconds since the record's origin.
     * It keeps which rows it already named, so a row's name event comes once, before the first
     * event on that row. It touches no file.
     */
    class SUSHIPROFILER_API ChromeTraceEvents final
    {
        public:
            /// @brief Returns the events that open a trace: the names of its two processes.
            [[nodiscard]] std::vector<std::string> opening() const;

            /**
             * @brief Returns the events of @p period: the period on its timeline row, then each
             *        zone run, sample and span, each row's name event before its first event.
             */
            [[nodiscard]] std::vector<std::string> events_of(
                const RunRecord& record, const PeriodRecord& period,
                const std::vector<ZoneRun>& instances);

        private:
            /// @brief Returns the host row of @p thread and @p lane, naming it on first use.
            std::int64_t host_row(const RunRecord& record, std::uint32_t thread, NameID lane,
                                  std::vector<std::string>& out);

            /// @brief Returns the host row of @p timeline, naming it on first use.
            std::int64_t timeline_row(const RunRecord& record, NameID timeline,
                                      std::vector<std::string>& out);

            /// @brief Names the row of device track @p track on first use.
            void name_track(const RunRecord& record, TrackID track, std::vector<std::string>& out);

            /// @brief Adds @p period's samples to @p out.
            void add_samples(const RunRecord& record, const PeriodRecord& period,
                             std::vector<std::string>& out);

            /// @brief Adds @p period's spans to @p out.
            void add_spans(const RunRecord& record, const PeriodRecord& period,
                           std::vector<std::string>& out);

            /// @brief Host rows by thread and lane; a timeline row has thread UINT32_MAX.
            std::map<std::pair<std::uint32_t, NameID>, std::int64_t> rows_;

            /// @brief Device tracks already named.
            std::set<TrackID> named_tracks_;

            /// @brief The next row id for a lane or timeline.
            std::int64_t next_row_ = 1'000'000;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
