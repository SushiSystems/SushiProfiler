/**************************************************************************/
/* chrome_trace_writer.cpp                                                */
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
 * @file chrome_trace_writer.cpp
 * @brief Writes the events of ChromeTraceEvents to a file as one Chrome trace object.
 * @author Mustafa Garip
 * @see modules/session/stream_writers/README.md
 */

#include <SushiProfiler/stream_writers/chrome_trace_writer.hpp>


namespace SushiProfiler
{
    ChromeTraceWriter::ChromeTraceWriter(std::string path) : path_(std::move(path))
    {

    }

    WriteStatus ChromeTraceWriter::begin(const RunRecord& record)
    {
        static_cast<void>(record);
        file_.open(path_, std::ios::binary | std::ios::trunc);
        if (!file_)
            return WriteStatus::PathUnwritable;
        file_ << "{\"displayTimeUnit\": \"ns\", \"traceEvents\": [";
        emit_all(events_.opening());
        return status();
    }

    WriteStatus ChromeTraceWriter::write_period(const RunRecord& record,
                                                const PeriodRecord& period,
                                                const std::vector<ZoneRun>& instances)
    {
        if (!file_.is_open())
            return WriteStatus::StreamFailed;
        emit_all(events_.events_of(record, period, instances));
        return status();
    }

    WriteStatus ChromeTraceWriter::end(const RunRecord& record)
    {
        static_cast<void>(record);
        if (!file_.is_open())
            return WriteStatus::StreamFailed;
        file_ << "\n]}\n";
        file_.close();
        return file_ ? WriteStatus::Written : WriteStatus::StreamFailed;
    }

    void ChromeTraceWriter::emit_all(const std::vector<std::string>& events)
    {
        for (const std::string& event : events)
        {
            file_ << (first_ ? "\n" : ",\n") << event;
            first_ = false;
        }
    }

    WriteStatus ChromeTraceWriter::status() const
    {
        return file_ ? WriteStatus::Written : WriteStatus::StreamFailed;
    }
} // namespace SushiProfiler
