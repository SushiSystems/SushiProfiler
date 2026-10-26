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
/* Licensed under the PolyForm Noncommercial License 1.0.0 (the           */
/* "License"); you may not use this file except in compliance with the    */
/* License. You may obtain a copy of the License at                       */
/*                                                                        */
/*     https://polyformproject.org/licenses/noncommercial/1.0.0           */
/*                                                                        */
/* Noncommercial use is free. Commercial use requires a separate licence  */
/* from Sushi Systems; see COMMERCIAL.md. The software is provided        */
/* "as is", without warranty of any kind.                                 */
/**************************************************************************/

/**
 * @file chrome_trace_writer.hpp
 * @brief Declares the writer of a Chrome trace that Perfetto and chrome://tracing open.
 * @author Mustafa Garip
 * @see modules/session/stream_writers/README.md
 */

#pragma once

#include <SushiProfiler/stream_writers/stream_writer.hpp>

#include <cstdint>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace SushiProfiler
{
    /**
     * @brief Streams a run as Chrome trace JSON: one event array, written period by period.
     *
     * Process 1 holds the host: one row per thread, one per lane under it and one per timeline.
     * Process 2 holds one row per device track. Times are microseconds since the record's origin.
     */
    class ChromeTraceWriter final : public StreamWriter
    {
        public:
            /// @brief Builds a writer of the file at @p path.
            explicit ChromeTraceWriter(std::string path);

            /// @brief Opens the file and writes the process names.
            [[nodiscard]] WriteStatus begin(const RunRecord& record) override;

            /// @brief Writes the period, its zone runs, samples and spans.
            [[nodiscard]] WriteStatus write_period(
                const RunRecord& record, const PeriodRecord& period,
                const std::vector<ZoneInstance>& instances) override;

            /// @brief Closes the event array and the file.
            [[nodiscard]] WriteStatus end(const RunRecord& record) override;

        private:
            /// @brief Writes one event object, @p event, after a separator.
            void emit(std::string_view event);

            /// @brief Returns the host row of @p thread and @p lane, naming it on first use.
            std::int64_t host_row(const RunRecord& record, std::uint32_t thread, NameID lane);

            /// @brief Returns the host row of @p timeline, naming it on first use.
            std::int64_t timeline_row(const RunRecord& record, NameID timeline);

            /// @brief Names the row of device track @p track on first use.
            void name_track(const RunRecord& record, TrackID track);

            /// @brief Writes @p period's samples.
            void write_samples(const RunRecord& record, const PeriodRecord& period);

            /// @brief Writes @p period's spans.
            void write_spans(const RunRecord& record, const PeriodRecord& period);

            /// @brief Returns the status the file's state gives.
            [[nodiscard]] WriteStatus status() const;

            /// @brief Where the trace goes.
            std::string path_;

            /// @brief The open trace.
            std::ofstream file_;

            /// @brief Whether no event has been written yet.
            bool first_ = true;

            /// @brief Host rows by thread and lane; a timeline row has thread UINT32_MAX.
            std::map<std::pair<std::uint32_t, NameID>, std::int64_t> rows_;

            /// @brief Device tracks already named.
            std::set<TrackID> named_tracks_;

            /// @brief The next row id for a lane or timeline.
            std::int64_t next_row_ = 1'000'000;
    };
} // namespace SushiProfiler
