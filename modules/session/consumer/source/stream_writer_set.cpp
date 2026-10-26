/**************************************************************************/
/* stream_writer_set.cpp                                                  */
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
 * @file stream_writer_set.cpp
 * @brief Begins, writes and ends stream writers as requests and failures change their state.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#include <SushiProfiler/consumer/stream_writer_set.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns @p status as the words a problem uses.
        const char* status_words(WriteStatus status) noexcept
        {
            return status == WriteStatus::PathUnwritable ? "its file could not be opened"
                                                         : "its stream failed";
        }
    } // namespace

    StreamWriterSet::StreamWriterSet(std::vector<Entry> entries)
    {
        for (Entry& entry : entries)
        {
            names_.push_back(std::move(entry.name));
            State state;
            state.writer = entry.writer;
            state.enabled = entry.enabled;
            states_.push_back(state);
        }
    }

    bool StreamWriterSet::knows(std::string_view name) const
    {
        for (const std::string& known : names_)
        {
            if (known == name)
                return true;
        }
        return false;
    }

    bool StreamWriterSet::request(std::string_view name, bool enabled)
    {
        for (std::size_t index = 0U; index < names_.size(); ++index)
        {
            if (names_[index] != name)
                continue;
            const std::lock_guard<std::mutex> lock(mutex_);
            requests_.emplace_back(index, enabled);
            return true;
        }
        return false;
    }

    std::string StreamWriterSet::begin_enabled(const RunRecord& record)
    {
        for (std::size_t index = 0U; index < states_.size(); ++index)
        {
            State& state = states_[index];
            if (!state.enabled || state.begun)
                continue;
            const WriteStatus status = state.writer->begin(record);
            if (status != WriteStatus::Written)
            {
                state.failed = true;
                return "writer \"" + names_[index] + "\" did not start: " + status_words(status);
            }
            state.begun = true;
        }
        return std::string();
    }

    void StreamWriterSet::apply_requests(const RunRecord& record,
                                         std::vector<std::string>& problems)
    {
        std::vector<std::pair<std::size_t, bool>> taken;
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            taken.swap(requests_);
        }
        for (const auto& [index, enabled] : taken)
        {
            State& state = states_[index];
            if (state.failed)
                continue;
            state.enabled = enabled;
            if (!enabled || state.begun)
                continue;
            const WriteStatus status = state.writer->begin(record);
            if (status != WriteStatus::Written)
            {
                state.failed = true;
                problems.push_back("writer \"" + names_[index] + "\" did not start when enabled: " +
                                   status_words(status));
                continue;
            }
            state.begun = true;
        }
    }

    void StreamWriterSet::write_period(const RunRecord& record, const PeriodRecord& period,
                                       const std::vector<ZoneRun>& runs,
                                       std::vector<std::string>& problems)
    {
        for (std::size_t index = 0U; index < states_.size(); ++index)
        {
            State& state = states_[index];
            if (!state.enabled || !state.begun || state.failed)
                continue;
            if (state.writer->write_period(record, period, runs) == WriteStatus::Written)
                continue;
            state.failed = true;
            problems.push_back("writer \"" + names_[index] + "\" failed on period " +
                               std::to_string(period.index) + " and was stopped");
        }
    }

    void StreamWriterSet::end_all(const RunRecord& record, std::vector<std::string>& problems)
    {
        for (std::size_t index = 0U; index < states_.size(); ++index)
        {
            State& state = states_[index];
            if (!state.begun)
                continue;
            state.begun = false;
            const WriteStatus status = state.writer->end(record);
            if (status != WriteStatus::Written)
                problems.push_back("writer \"" + names_[index] + "\" did not finish: " +
                                   status_words(status));
        }
    }
} // namespace SushiProfiler
