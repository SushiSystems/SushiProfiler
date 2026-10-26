/**************************************************************************/
/* table.hpp                                                              */
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
 * @file table.hpp
 * @brief Declares the table every analysis returns: named columns with units, rows of cells.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace SushiProfiler
{
    /// @brief Names what a column's numbers measure, so a writer can format them.
    enum class Unit : std::uint8_t
    {
        None,        /**< Text, or a number with no unit. */
        Nanoseconds, /**< A time. */
        Count,       /**< A number of things. */
        Ratio        /**< A fraction, 1 for the whole. */
    };

    /// @brief One column of a table.
    struct Column
    {
        std::string name;     /**< The column's name. */
        Unit unit = Unit::None; /**< What its numbers measure. */

        /// @brief Returns whether name and unit are equal.
        [[nodiscard]] bool operator==(const Column& other) const;
    };

    /// @brief One cell: empty, text, a whole number or a real.
    class Cell final
    {
        public:
            /// @brief Builds an empty cell.
            Cell() noexcept = default;

            /// @brief Returns a text cell.
            [[nodiscard]] static Cell text(std::string value);

            /// @brief Returns a whole-number cell.
            [[nodiscard]] static Cell whole(std::int64_t value);

            /// @brief Returns a real cell.
            [[nodiscard]] static Cell real(double value);

            /// @brief Returns whether the cell holds nothing.
            [[nodiscard]] bool empty() const noexcept;

            /// @brief Returns the text, or null when the cell holds none.
            [[nodiscard]] const std::string* as_text() const noexcept;

            /// @brief Returns the whole number, or null when the cell holds none.
            [[nodiscard]] const std::int64_t* as_whole() const noexcept;

            /// @brief Returns the real, or null when the cell holds none.
            [[nodiscard]] const double* as_real() const noexcept;

            /// @brief Returns whether kind and content are equal.
            [[nodiscard]] bool operator==(const Cell& other) const;

            /// @brief Returns whether kind or content differ.
            [[nodiscard]] bool operator!=(const Cell& other) const;

        private:
            /// @brief The content.
            std::variant<std::monostate, std::string, std::int64_t, double> content_;
    };

    /// @brief One row of cells, one per column.
    using Row = std::vector<Cell>;

    /// @brief What one analysis found: its columns, its rows in order, and notes on the whole.
    struct Table
    {
        std::string name;                /**< The analysis that made it. */
        std::vector<Column> columns;     /**< The columns, in order. */
        std::vector<Row> rows;           /**< The rows, each with one cell per column. */
        std::vector<std::string> notes;  /**< Sentences about the table as a whole. */

        /// @brief Appends a column named @p column_name measuring @p unit.
        void add_column(std::string column_name, Unit unit = Unit::None);

        /// @brief Returns the index of the column named @p column_name, or columns.size().
        [[nodiscard]] std::size_t column(const std::string& column_name) const noexcept;
    };
} // namespace SushiProfiler
