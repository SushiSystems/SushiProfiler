/**************************************************************************/
/* short_spans_analysis.cpp                                               */
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
 * @file short_spans_analysis.cpp
 * @brief Counts the spans under a bound and prices them at the launch floor.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/short_spans_analysis.hpp>

#include <utility>

#include "record_queries.hpp"

namespace SushiProfiler
{
    std::string_view ShortSpansAnalysis::name() const noexcept
    {
        return "short_spans";
    }

    Table ShortSpansAnalysis::run(const RunRecord& record, const AnalysisOptions& options) const
    {
        Table table;
        table.name = "short_spans";
        table.add_column("bound", Unit::Nanoseconds);
        table.add_column("spans", Unit::Count);
        table.add_column("short", Unit::Count);
        table.add_column("short_share", Unit::Ratio);
        table.add_column("short_time", Unit::Nanoseconds);
        table.add_column("short_time_share", Unit::Ratio);
        table.add_column("launch_floor", Unit::Nanoseconds);
        table.add_column("floor_cost", Unit::Nanoseconds);
        std::int64_t spans = 0;
        std::int64_t short_spans = 0;
        double time = 0.0;
        double short_time = 0.0;
        for (const PeriodRecord* const period : Detail::active_periods(record))
        {
            for (std::size_t row = 0U; row < period->spans.size(); ++row)
            {
                const Span span = period->spans.span(row);
                const std::uint64_t length = span.end_ns - span.start_ns;
                ++spans;
                time += static_cast<double>(length);
                if (length >= options.short_span_ns)
                    continue;
                ++short_spans;
                short_time += static_cast<double>(length);
            }
        }
        const auto floor = static_cast<double>(options.launch_floor_ns);
        Row row{Cell::real(static_cast<double>(options.short_span_ns)), Cell::whole(spans),
                Cell::whole(short_spans),
                Cell::real(spans > 0 ? static_cast<double>(short_spans) / static_cast<double>(spans)
                                     : 0.0),
                Cell::real(short_time), Cell::real(time > 0.0 ? short_time / time : 0.0),
                Cell::real(floor), Cell::real(floor * static_cast<double>(short_spans))};
        table.rows.push_back(std::move(row));
        if (options.launch_floor_ns == 0U)
            table.notes.emplace_back("The launch floor is unknown, so the floor cost is 0.");
        return table;
    }
} // namespace SushiProfiler
