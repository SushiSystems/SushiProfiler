/**************************************************************************/
/* memory_stream_writer.cpp                                               */
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
 * @file memory_stream_writer.cpp
 * @brief Keeps the periods handed to the in-memory stream writer.
 * @author Mustafa Garip
 * @see modules/session/stream_writers/README.md
 */

#include <SushiProfiler/stream_writers/memory_stream_writer.hpp>

#include <utility>

namespace SushiProfiler
{
    void MemoryStreamWriter::fail_with(WriteStatus status) noexcept
    {
        status_ = status;
    }

    void MemoryStreamWriter::watch_writes(
        std::function<void(const RunRecord&, const PeriodRecord&)> watcher)
    {
        watcher_ = std::move(watcher);
    }

    WriteStatus MemoryStreamWriter::begin(const RunRecord& record)
    {
        static_cast<void>(record);
        ++begins_;
        return status_;
    }

    WriteStatus MemoryStreamWriter::write_period(const RunRecord& record,
                                                 const PeriodRecord& period,
                                                 const std::vector<ZoneRun>& instances)
    {
        if (watcher_)
            watcher_(record, period);
        ++writes_;
        if (status_ != WriteStatus::Written)
            return status_;
        periods_.push_back(period);
        instances_.push_back(instances);
        return status_;
    }

    WriteStatus MemoryStreamWriter::end(const RunRecord& record)
    {
        static_cast<void>(record);
        ++ends_;
        return status_;
    }

    std::size_t MemoryStreamWriter::begins() const noexcept
    {
        return begins_;
    }

    std::size_t MemoryStreamWriter::ends() const noexcept
    {
        return ends_;
    }

    std::size_t MemoryStreamWriter::writes() const noexcept
    {
        return writes_;
    }

    const std::vector<PeriodRecord>& MemoryStreamWriter::periods() const noexcept
    {
        return periods_;
    }

    const std::vector<std::vector<ZoneRun>>& MemoryStreamWriter::instances() const noexcept
    {
        return instances_;
    }
} // namespace SushiProfiler
