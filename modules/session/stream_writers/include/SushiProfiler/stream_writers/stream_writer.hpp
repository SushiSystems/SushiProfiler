/**************************************************************************/
/* stream_writer.hpp                                                      */
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
 * @file stream_writer.hpp
 * @brief Declares the interface of a writer the session feeds one period at a time.
 * @author Mustafa Garip
 * @see modules/session/stream_writers/README.md
 */

#pragma once

#include <SushiProfiler/fold/period_fold.hpp>
#include <SushiProfiler/record/period_record.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/record_writers/record_writer.hpp>

#include <vector>

namespace SushiProfiler
{
    /**
     * @brief Writes each period as it closes, so memory holds one period, not the run.
     *
     * The session calls begin() once, write_period() once per recorded period in order, and end()
     * once. @p record is the run so far: its names, sites, tracks and threads cover every id the
     * period uses.
     */
    class StreamWriter
    {
        public:
            virtual ~StreamWriter() = default;

            /// @brief Opens the output.
            [[nodiscard]] virtual WriteStatus begin(const RunRecord& record) = 0;

            /// @brief Writes @p period and the zone runs it folded from.
            [[nodiscard]] virtual WriteStatus write_period(
                const RunRecord& record, const PeriodRecord& period,
                const std::vector<ZoneInstance>& instances) = 0;

            /// @brief Finishes and closes the output.
            [[nodiscard]] virtual WriteStatus end(const RunRecord& record) = 0;

        protected:
            StreamWriter() = default;
            StreamWriter(const StreamWriter&) = default;
            StreamWriter& operator=(const StreamWriter&) = default;
            StreamWriter(StreamWriter&&) = default;
            StreamWriter& operator=(StreamWriter&&) = default;
    };
} // namespace SushiProfiler
