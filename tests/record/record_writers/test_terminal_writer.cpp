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
 * @file test_terminal_writer.cpp
 * @brief Tests the terminal writer's exact text and its cell formats per unit.
 * @author Mustafa Garip
 * @see tests/record/record_writers/README.md
 */

#include <SushiProfiler/record_writers/terminal_writer.hpp>

#include <gtest/gtest.h>

#include <sstream>
#include <string>

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

    /// @brief A cell, its unit and the text a terminal shows for them.
    struct FormatRow
    {
        Cell cell;
        Unit unit;
        const char* text;
    };
} // namespace

TEST(Small_TerminalWriter, AlignsEachColumnToTheWidestRowItPrints)
{
    std::ostringstream out;
    TerminalWriter writer(out, 1U);

    static_cast<void>(writer.write(RunRecord{}, {demo_table()}));

    const std::string expected = "== demo ==\n"
                                 "subject  count   median  share  value\n"
                                 "conv         3  1.50 us  25.0%    2.5\n";
    EXPECT_EQ(out.str().rfind(expected, 0U), 0U) << out.str();
}

TEST(Small_TerminalWriter, CountsTheRowsItCuts)
{
    std::ostringstream out;
    TerminalWriter writer(out, 1U);

    static_cast<void>(writer.write(RunRecord{}, {demo_table()}));

    EXPECT_NE(out.str().find("2.5\n(1 more row)\n"), std::string::npos) << out.str();
}

TEST(Small_TerminalWriter, PrintsANoteAfterTheRows)
{
    std::ostringstream out;
    TerminalWriter writer(out);

    static_cast<void>(writer.write(RunRecord{}, {demo_table()}));

    EXPECT_NE(out.str().find("12345.7\nnote: Example note.\n"), std::string::npos) << out.str();
}

TEST(Small_TerminalWriter, ATableWithoutRowsPrintsNoRows)
{
    std::ostringstream out;
    Table empty;
    empty.name = "empty";
    empty.add_column("x");
    TerminalWriter writer(out);

    static_cast<void>(writer.write(RunRecord{}, {empty}));

    EXPECT_EQ(out.str(), "== empty ==\n"
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

class Small_TerminalWriterFormat : public ::testing::TestWithParam<FormatRow>
{
};

TEST_P(Small_TerminalWriterFormat, ACellIsFormattedByItsUnit)
{
    EXPECT_EQ(format_cell(GetParam().cell, GetParam().unit), GetParam().text);
}

INSTANTIATE_TEST_SUITE_P(
    Small_Units, Small_TerminalWriterFormat,
    ::testing::Values(FormatRow{Cell::real(999.0), Unit::Nanoseconds, "999 ns"},
                      FormatRow{Cell::real(1500.0), Unit::Nanoseconds, "1.50 us"},
                      FormatRow{Cell::real(2.5e6), Unit::Nanoseconds, "2.50 ms"},
                      FormatRow{Cell::real(3.25e9), Unit::Nanoseconds, "3.250 s"},
                      FormatRow{Cell::whole(12), Unit::Nanoseconds, "12 ns"},
                      FormatRow{Cell::real(0.125), Unit::Ratio, "12.5%"},
                      FormatRow{Cell::real(4.0), Unit::Count, "4"},
                      FormatRow{Cell::real(3.0), Unit::None, "3"},
                      FormatRow{Cell::real(0.123456), Unit::None, "0.1235"},
                      FormatRow{Cell::real(12345.678), Unit::None, "12345.7"},
                      FormatRow{Cell(), Unit::None, ""}));
