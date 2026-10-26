/**************************************************************************/
/* terminal_writer.cpp                                                    */
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
 * @file terminal_writer.cpp
 * @brief Formats cells by unit, measures each column and prints the tables aligned.
 * @author Mustafa Garip
 * @see modules/record/record_writers/README.md
 */

#include <SushiProfiler/record_writers/terminal_writer.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns @p value printed by printf @p format.
        std::string printed(const char* format, double value)
        {
            char buffer[64];
            const int length = std::snprintf(buffer, sizeof(buffer), format, value);
            return std::string(buffer, static_cast<std::size_t>(std::max(length, 0)));
        }

        /// @brief Returns a time in the largest of ns, us, ms and s that keeps it at or above 1.
        std::string time_text(double ns)
        {
            const double size = std::fabs(ns);
            if (size < 1e3)
                return printed("%.0f ns", ns);
            if (size < 1e6)
                return printed("%.2f us", ns / 1e3);
            if (size < 1e9)
                return printed("%.2f ms", ns / 1e6);
            return printed("%.3f s", ns / 1e9);
        }

        /// @brief Returns a number with no unit: whole when it is, else four significant digits.
        std::string plain_text(double value)
        {
            if (std::fabs(value) >= 1e3 || value == std::floor(value))
                return printed(value == std::floor(value) ? "%.0f" : "%.1f", value);
            return printed("%.4g", value);
        }

        /// @brief Returns @p line without trailing spaces.
        std::string trimmed(std::string line)
        {
            while (!line.empty() && line.back() == ' ')
                line.pop_back();
            return line;
        }

        /// @brief Returns @p cells laid out in columns of @p widths, aligned by @p left.
        std::string laid_out(const std::vector<std::string>& cells,
                             const std::vector<std::size_t>& widths, const std::vector<bool>& left)
        {
            std::string line;
            for (std::size_t column = 0U; column < cells.size(); ++column)
            {
                if (column != 0U)
                    line += "  ";
                const std::string padding(widths[column] - cells[column].size(), ' ');
                line += left[column] ? cells[column] + padding : padding + cells[column];
            }
            return trimmed(std::move(line));
        }

        /// @brief Prints @p table to @p out, at most @p limit rows.
        void print_table(std::ostream& out, const Table& table, std::size_t limit)
        {
            out << "== " << table.name << " ==\n";
            const std::size_t shown = std::min(limit, table.rows.size());
            std::vector<std::vector<std::string>> lines(1U);
            std::vector<std::size_t> widths;
            std::vector<bool> left;
            for (const Column& column : table.columns)
            {
                lines[0].push_back(column.name);
                widths.push_back(column.name.size());
                left.push_back(true);
            }
            for (std::size_t row = 0U; row < shown; ++row)
            {
                std::vector<std::string> cells;
                for (std::size_t column = 0U; column < table.columns.size(); ++column)
                {
                    const Cell& cell = table.rows[row][column];
                    cells.push_back(format_cell(cell, table.columns[column].unit));
                    widths[column] = std::max(widths[column], cells.back().size());
                    if (row == 0U)
                        left[column] = cell.as_text() != nullptr || cell.empty();
                }
                lines.push_back(std::move(cells));
            }
            for (const std::vector<std::string>& cells : lines)
                out << laid_out(cells, widths, left) << '\n';
            if (table.rows.empty())
                out << "(no rows)\n";
            const std::size_t hidden = table.rows.size() - shown;
            if (hidden != 0U)
                out << '(' << hidden << (hidden == 1U ? " more row)\n" : " more rows)\n");
            for (const std::string& note : table.notes)
                out << "note: " << note << '\n';
        }
    } // namespace

    std::string format_cell(const Cell& cell, Unit unit)
    {
        if (const std::string* const text = cell.as_text())
            return *text;
        if (const std::int64_t* const whole = cell.as_whole())
            return unit == Unit::Nanoseconds ? time_text(static_cast<double>(*whole))
                                             : std::to_string(*whole);
        const double* const real = cell.as_real();
        if (real == nullptr)
            return std::string();
        switch (unit)
        {
            case Unit::Nanoseconds: return time_text(*real);
            case Unit::Ratio: return printed("%.1f%%", *real * 100.0);
            case Unit::Count: return printed("%.0f", *real);
            case Unit::None: return plain_text(*real);
        }
        return plain_text(*real);
    }

    TerminalWriter::TerminalWriter(std::ostream& out, std::size_t rows) noexcept
        : out_(out), rows_(rows)
    {

    }

    WriteStatus TerminalWriter::write(const RunRecord& record, const std::vector<Table>& tables)
    {
        static_cast<void>(record);
        for (std::size_t index = 0U; index < tables.size(); ++index)
        {
            if (index != 0U)
                out_ << '\n';
            print_table(out_, tables[index], rows_);
        }
        out_.flush();
        return out_ ? WriteStatus::Written : WriteStatus::StreamFailed;
    }
} // namespace SushiProfiler
