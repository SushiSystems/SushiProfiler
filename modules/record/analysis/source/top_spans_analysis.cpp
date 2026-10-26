/**************************************************************************/
/* top_spans_analysis.cpp                                                 */
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
 * @file top_spans_analysis.cpp
 * @brief Groups spans by name and chosen attributes and ranks the groups by total time.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/top_spans_analysis.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <SushiProfiler/statistics/sorted_samples.hpp>

#include "record_queries.hpp"

namespace SushiProfiler
{
    namespace
    {
        /// @brief The text of an attribute a span does not have.
        constexpr const char* K_ABSENT = "-";

        /// @brief Returns the text of @p column's value at @p row.
        std::string attribute_text(const RunRecord& record, const AttributeColumn* column,
                                   std::size_t row)
        {
            if (column == nullptr || column->present[row] == 0U)
                return K_ABSENT;
            if (column->kind == AttributeKind::Name)
                return std::string(record.name_of(static_cast<NameID>(column->integers[row])));
            if (column->kind == AttributeKind::Integer)
                return std::to_string(column->integers[row]);
            std::array<char, 32> buffer{};
            const auto written =
                std::to_chars(buffer.data(), buffer.data() + buffer.size(), column->reals[row]);
            return std::string(buffer.data(), written.ptr);
        }
    } // namespace

    std::string_view TopSpansAnalysis::name() const noexcept
    {
        return "top_spans";
    }

    Table TopSpansAnalysis::run(const RunRecord& record, const AnalysisOptions& options) const
    {
        Table table;
        table.name = "top_spans";
        table.add_column("span");
        for (const std::string& key : options.group_by)
            table.add_column(key);
        table.add_column("count", Unit::Count);
        table.add_column("total", Unit::Nanoseconds);
        table.add_column("median", Unit::Nanoseconds);
        table.add_column("share", Unit::Ratio);
        std::map<std::vector<std::string>, std::vector<double>> groups;
        double total = 0.0;
        for (const PeriodRecord* const period : Detail::active_periods(record))
        {
            std::vector<const AttributeColumn*> columns;
            for (const std::string& key : options.group_by)
            {
                const NameID id = record.find_name(key);
                columns.push_back(id == 0U ? nullptr : period->spans.attribute(id));
            }
            for (std::size_t row = 0U; row < period->spans.size(); ++row)
            {
                const Span span = period->spans.span(row);
                std::vector<std::string> label{std::string(record.name_of(span.name))};
                for (const AttributeColumn* const column : columns)
                    label.push_back(attribute_text(record, column, row));
                const auto length = static_cast<double>(span.end_ns - span.start_ns);
                groups[label].push_back(length);
                total += length;
            }
        }
        std::vector<std::pair<std::vector<std::string>, std::vector<double>>> ranked(
            groups.begin(), groups.end());
        const auto sum = [](const std::vector<double>& lengths)
        {
            double value = 0.0;
            for (const double length : lengths)
                value += length;
            return value;
        };
        std::stable_sort(ranked.begin(), ranked.end(), [&sum](const auto& left, const auto& right)
                         {
                             return sum(left.second) > sum(right.second);
                         });
        const std::size_t kept = std::min(options.top, ranked.size());
        for (std::size_t index = 0U; index < kept; ++index)
        {
            const auto& [label, lengths] = ranked[index];
            Row row;
            for (const std::string& part : label)
                row.push_back(Cell::text(part));
            const double group_total = sum(lengths);
            row.push_back(Cell::whole(static_cast<std::int64_t>(lengths.size())));
            row.push_back(Cell::real(group_total));
            row.push_back(Cell::real(SortedSamples(lengths).median()));
            row.push_back(Cell::real(total > 0.0 ? group_total / total : 0.0));
            table.rows.push_back(std::move(row));
        }
        const std::size_t hidden = ranked.size() - kept;
        if (hidden != 0U)
        {
            table.notes.push_back(std::to_string(hidden) +
                                  (hidden == 1U ? " group is" : " groups are") + " not shown.");
        }
        return table;
    }
} // namespace SushiProfiler
