/**************************************************************************/
/* stream_writer_set.hpp                                                  */
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
 * @file stream_writer_set.hpp
 * @brief Declares the set of stream writers a session feeds, each switchable at a period boundary.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#pragma once

#include <SushiProfiler/record/period_record.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/stream_writers/stream_writer.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Holds the stream writers of a session and which of them receive periods.
     *
     * Any thread may request a writer on or off. The consumer thread alone writes, begins and
     * ends them: a request takes effect when the consumer calls apply_requests(), at the start of
     * a period's close, so a writer never changes state in the middle of a period. A writer that
     * starts disabled is not begun, and so opens no output, until it is first enabled.
     */
    class SUSHIPROFILER_API StreamWriterSet final
    {
        public:
            /// @brief One writer the configuration named.
            struct Entry
            {
                std::string name;               /**< The name the configuration gave it. */
                StreamWriter* writer = nullptr; /**< The writer; it outlives the set. */
                bool enabled = true;            /**< Whether it receives periods from the start. */
            };

            /// @brief Builds a set of @p entries; no writer is begun yet.
            explicit StreamWriterSet(std::vector<Entry> entries);

            StreamWriterSet(const StreamWriterSet&) = delete;
            StreamWriterSet& operator=(const StreamWriterSet&) = delete;
            StreamWriterSet(StreamWriterSet&&) = delete;
            StreamWriterSet& operator=(StreamWriterSet&&) = delete;
            ~StreamWriterSet() = default;

            /// @brief Returns whether a writer is named @p name.
            [[nodiscard]] bool knows(std::string_view name) const;

            /// @brief Asks for writer @p name to be on or off from the next close; any thread.
            /// @return false when no writer has that name.
            [[nodiscard]] bool request(std::string_view name, bool enabled);

            /// @brief Begins every writer enabled at start; returns the first failure, or empty.
            [[nodiscard]] std::string begin_enabled(const RunRecord& record);

            /// @brief Applies the requests made since the last call; the consumer thread calls it.
            void apply_requests(const RunRecord& record, std::vector<std::string>& problems);

            /// @brief Writes @p period to every enabled writer; one that fails is stopped.
            void write_period(const RunRecord& record, const PeriodRecord& period,
                              const std::vector<ZoneRun>& runs,
                              std::vector<std::string>& problems);

            /// @brief Ends every writer that began, once.
            void end_all(const RunRecord& record, std::vector<std::string>& problems);

        private:
            /// @brief One writer and where it stands.
            struct State
            {
                StreamWriter* writer = nullptr; /**< The writer. */
                bool enabled = true;            /**< Whether it receives periods. */
                bool begun = false;             /**< Whether begin() succeeded and end() has not run. */
                bool failed = false;            /**< Whether it failed and was stopped. */
            };

            std::vector<std::string> names_; /**< The writers' names; fixed at construction. */
            std::vector<State> states_;      /**< One state per name; the consumer thread's. */
            mutable std::mutex mutex_;       /**< Guards requests_. */
            /// @brief Writer index and wanted state, in the order requested.
            std::vector<std::pair<std::size_t, bool>> requests_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
