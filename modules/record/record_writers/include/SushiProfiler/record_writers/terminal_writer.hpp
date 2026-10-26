/**************************************************************************/
/* terminal_writer.hpp                                                    */
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
 * @file terminal_writer.hpp
 * @brief Declares the writer that prints analysis tables as aligned text.
 * @author Mustafa Garip
 * @see modules/record/record_writers/README.md
 */

#pragma once

#include <SushiProfiler/record_writers/record_writer.hpp>

#include <cstddef>
#include <ostream>
#include <string>

namespace SushiProfiler
{
    /**
     * @brief Prints each table under its name, columns aligned, units in readable steps.
     *
     * Times print in ns, us, ms or s, ratios as percentages. A text column is left-aligned and a
     * number column right-aligned; rows past the limit are counted, not printed.
     */
    class TerminalWriter final : public RecordWriter
    {
        public:
            /// @brief Builds a writer to @p out, which outlives it, printing @p rows rows a table.
            explicit TerminalWriter(std::ostream& out, std::size_t rows = 20U) noexcept;

            /// @brief Prints @p tables; the record itself is not printed.
            [[nodiscard]] WriteStatus write(const RunRecord& record,
                                            const std::vector<Table>& tables) override;

        private:
            /// @brief The stream printed to.
            std::ostream& out_;

            /// @brief The most rows printed per table.
            std::size_t rows_;
    };

    /// @brief Returns @p cell as the terminal writer prints it in a column of @p unit.
    [[nodiscard]] std::string format_cell(const Cell& cell, Unit unit);
} // namespace SushiProfiler
