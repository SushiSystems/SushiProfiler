/**************************************************************************/
/* roofline_analysis.cpp                                                  */
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
 * @file roofline_analysis.cpp
 * @brief Sums the cost attributes of spans and compares their rates with the device's ceilings.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/roofline_analysis.hpp>

#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <utility>

#include "record_queries.hpp"

namespace SushiProfiler
{
    namespace
    {
        /// @brief The key of a track's compute ceiling is `device.<track>.` plus this.
        constexpr const char* K_PEAK_FLOPS = "peak_flops";

        /// @brief The key of a track's memory ceiling is `device.<track>.` plus this.
        constexpr const char* K_PEAK_BYTES = "peak_bytes_per_s";

        /// @brief The spans of one name on one track in one period, summed.
        struct Totals
        {
            std::int64_t spans = 0;       /**< Spans that carry both attributes. */
            std::uint64_t time_ns = 0U;   /**< Their device time. */
            std::uint64_t flops = 0U;     /**< Their `flops`. */
            std::uint64_t bytes = 0U;     /**< Their `bytes`. */
        };

        /// @brief Returns attribute column @p key of @p period when it holds integers, or null.
        const AttributeColumn* integer_column(const PeriodRecord& period, NameID key)
        {
            if (key == 0U)
                return nullptr;
            const AttributeColumn* const column = period.spans.attribute(key);
            return column != nullptr && column->kind == AttributeKind::Integer ? column : nullptr;
        }

        /// @brief Returns whether row @p row of @p column holds a value that is not negative.
        bool has_count(const AttributeColumn* column, std::size_t row)
        {
            return column != nullptr && column->present[row] == 1U && column->integers[row] >= 0;
        }

        /// @brief Returns the positive number environment fact @p name holds, or 0 when it does
        /// not hold one.
        double ceiling(const RunRecord& record, const std::string& name)
        {
            for (const Fact& fact : record.environment)
            {
                if (fact.name != name)
                    continue;
                char* end = nullptr;
                const double value = std::strtod(fact.value.c_str(), &end);
                const bool whole = end != fact.value.c_str() && *end == '\0';
                return whole && std::isfinite(value) && value > 0.0 ? value : 0.0;
            }
            return 0.0;
        }

        /// @brief Returns the name of track @p track, or `track <id>`.
        std::string track_label(const RunRecord& record, TrackID track)
        {
            if (track < record.tracks.size() && !record.tracks[track].empty())
                return record.tracks[track];
            return "track " + std::to_string(track);
        }
    } // namespace

    std::string_view RooflineAnalysis::name() const noexcept
    {
        return "roofline";
    }

    Table RooflineAnalysis::run(const RunRecord& record, const AnalysisOptions& options) const
    {
        static_cast<void>(options);
        Table table;
        table.name = "roofline";
        table.add_column("period", Unit::Count);
        table.add_column("name");
        table.add_column("track");
        table.add_column("spans", Unit::Count);
        table.add_column("time", Unit::Nanoseconds);
        table.add_column("flops", Unit::Count);
        table.add_column("bytes", Unit::Count);
        table.add_column("flops_per_s");
        table.add_column("bytes_per_s");
        table.add_column("intensity");
        table.add_column("peak_flops");
        table.add_column("peak_bytes_per_s");
        table.add_column("bound_flops_per_s");
        table.add_column("bound_share", Unit::Ratio);
        table.add_column("limit");
        const NameID flops_key = record.find_name("flops");
        const NameID bytes_key = record.find_name("bytes");
        std::int64_t skipped = 0;
        for (const PeriodRecord* const period : Detail::active_periods(record))
        {
            const AttributeColumn* const flops = integer_column(*period, flops_key);
            const AttributeColumn* const bytes = integer_column(*period, bytes_key);
            std::map<std::pair<std::string, TrackID>, Totals> groups;
            for (std::size_t row = 0U; row < period->spans.size(); ++row)
            {
                if (!has_count(flops, row) || !has_count(bytes, row))
                {
                    ++skipped;
                    continue;
                }
                const Span span = period->spans.span(row);
                Totals& totals =
                    groups[{std::string(record.name_of(span.name)), span.track}];
                ++totals.spans;
                totals.time_ns += span.end_ns - span.start_ns;
                totals.flops += static_cast<std::uint64_t>(flops->integers[row]);
                totals.bytes += static_cast<std::uint64_t>(bytes->integers[row]);
            }
            for (const auto& [key, totals] : groups)
            {
                const std::string track = track_label(record, key.second);
                const auto flops_total = static_cast<double>(totals.flops);
                const auto bytes_total = static_cast<double>(totals.bytes);
                const double seconds = static_cast<double>(totals.time_ns) / 1e9;
                const double flops_rate = seconds > 0.0 ? flops_total / seconds : 0.0;
                const double bytes_rate = seconds > 0.0 ? bytes_total / seconds : 0.0;
                Row row{Cell::whole(static_cast<std::int64_t>(period->index)),
                        Cell::text(key.first), Cell::text(track), Cell::whole(totals.spans),
                        Cell::whole(static_cast<std::int64_t>(totals.time_ns)),
                        Cell::whole(static_cast<std::int64_t>(totals.flops)),
                        Cell::whole(static_cast<std::int64_t>(totals.bytes)),
                        Cell::real(flops_rate), Cell::real(bytes_rate)};
                const bool has_bytes = totals.bytes > 0U;
                row.push_back(has_bytes ? Cell::real(flops_total / bytes_total) : Cell());
                const double peak_flops = ceiling(record, "device." + track + "." + K_PEAK_FLOPS);
                const double peak_bytes = ceiling(record, "device." + track + "." + K_PEAK_BYTES);
                if (peak_flops > 0.0 && peak_bytes > 0.0)
                {
                    const double memory_roof =
                        has_bytes ? flops_total / bytes_total * peak_bytes : peak_flops;
                    const bool memory_bound = memory_roof < peak_flops;
                    const double bound = memory_bound ? memory_roof : peak_flops;
                    row.push_back(Cell::real(peak_flops));
                    row.push_back(Cell::real(peak_bytes));
                    row.push_back(Cell::real(bound));
                    row.push_back(Cell::real(flops_rate / bound));
                    row.push_back(Cell::text(memory_bound ? "memory" : "compute"));
                }
                else
                {
                    row.insert(row.end(), 5U, Cell());
                }
                table.rows.push_back(std::move(row));
            }
        }
        if (skipped > 0)
        {
            table.notes.push_back(std::to_string(skipped) +
                                  " spans without both flops and bytes were skipped.");
        }
        return table;
    }
} // namespace SushiProfiler
