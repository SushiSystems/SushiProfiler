/**************************************************************************/
/* test_terminal_writer.cpp                                               */
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
 * @file test_terminal_writer.cpp
 * @brief Tests the terminal writer's exact text and its cell formats per unit.
 * @author Mustafa Garip
 * @see tests/record/record_writers/README.md
 */

#include <SushiProfiler/record_writers/terminal_writer.hpp>

#include <gtest/gtest.h>

#include <sstream>

using SushiProfiler::Cell;
using SushiProfiler::format_cell;
using SushiProfiler::RunRecord;
using SushiProfiler::Table;
using SushiProfiler::TerminalWriter;
using SushiProfiler::Unit;
using SushiProfiler::WriteStatus;

namespace
{
    /// @brief Returns a two-row table that uses every unit.
    Table demo_table()
    {
        Table table;
        table.name = "demo";
        table.add_column("subject");
        table.add_column("count", Unit::Count);
        table.add_column("median", Unit::Nanoseconds);
        table.add_column("share", Unit::Ratio);
        table.add_column("value");
        table.rows.push_back({Cell::text("conv"), Cell::whole(3), Cell::real(1500.0),
                              Cell::real(0.25), Cell::real(2.5)});
        table.rows.push_back({Cell::text("forward pass"), Cell::whole(12), Cell::real(2.5e6),
                              Cell::real(0.5), Cell::real(12345.678)});
        table.notes.emplace_back("Example note.");
        return table;
    }
} // namespace

TEST(Small_TerminalWriter, PrintsAlignedTablesAndCountsTheRowsCut)
{
    std::ostringstream out;
    Table empty;
    empty.name = "empty";
    empty.add_column("x");
    TerminalWriter writer(out, 1U);

    EXPECT_EQ(writer.write(RunRecord{}, {demo_table(), empty}), WriteStatus::Written);
    EXPECT_EQ(out.str(), "== demo ==\n"
                         "subject  count   median  share  value\n"
                         "conv         3  1.50 us  25.0%    2.5\n"
                         "(1 more row)\n"
                         "note: Example note.\n"
                         "\n"
                         "== empty ==\n"
                         "x\n"
                         "(no rows)\n");
}

TEST(Small_TerminalWriter, WidensColumnsForEveryPrintedRow)
{
    std::ostringstream out;
    TerminalWriter writer(out);

    EXPECT_EQ(writer.write(RunRecord{}, {demo_table()}), WriteStatus::Written);
    EXPECT_NE(out.str().find("forward pass     12  2.50 ms  50.0%  12345.7\n"), std::string::npos)
        << out.str();
}

TEST(Small_TerminalWriter, FormatsTimesInReadableSteps)
{
    EXPECT_EQ(format_cell(Cell::real(999.0), Unit::Nanoseconds), "999 ns");
    EXPECT_EQ(format_cell(Cell::real(1500.0), Unit::Nanoseconds), "1.50 us");
    EXPECT_EQ(format_cell(Cell::real(2.5e6), Unit::Nanoseconds), "2.50 ms");
    EXPECT_EQ(format_cell(Cell::real(3.25e9), Unit::Nanoseconds), "3.250 s");
    EXPECT_EQ(format_cell(Cell::whole(12), Unit::Nanoseconds), "12 ns");
}

TEST(Small_TerminalWriter, FormatsRatiosCountsAndPlainNumbers)
{
    EXPECT_EQ(format_cell(Cell::real(0.125), Unit::Ratio), "12.5%");
    EXPECT_EQ(format_cell(Cell::real(4.0), Unit::Count), "4");
    EXPECT_EQ(format_cell(Cell::real(3.0), Unit::None), "3");
    EXPECT_EQ(format_cell(Cell::real(0.123456), Unit::None), "0.1235");
    EXPECT_EQ(format_cell(Cell::real(12345.678), Unit::None), "12345.7");
    EXPECT_EQ(format_cell(Cell(), Unit::None), "");
}
