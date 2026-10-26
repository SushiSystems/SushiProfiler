/**************************************************************************/
/* test_record_writer.cpp                                                 */
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
 * @file test_record_writer.cpp
 * @brief Tests that writers of either kind run through the one interface.
 * @author Mustafa Garip
 * @see tests/record/record_writers/README.md
 */

#include <SushiProfiler/record_writers/record_writer.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <vector>

#include <SushiProfiler/record_writers/terminal_writer.hpp>

using SushiProfiler::RecordWriter;
using SushiProfiler::RunRecord;
using SushiProfiler::Table;
using SushiProfiler::WriteStatus;

namespace
{
    /// @brief Counts the tables it is given.
    class CountingWriter final : public RecordWriter
    {
        public:
            WriteStatus write(const RunRecord&, const std::vector<Table>& tables) override
            {
                seen += tables.size();
                return WriteStatus::Written;
            }

            /// @brief The tables seen so far.
            std::size_t seen = 0U;
    };
} // namespace

TEST(Small_RecordWriter, WritersRunThroughTheInterface)
{
    std::ostringstream out;
    CountingWriter counting;
    std::vector<std::unique_ptr<RecordWriter>> writers;
    writers.push_back(std::make_unique<SushiProfiler::TerminalWriter>(out));
    std::vector<RecordWriter*> all = {writers[0].get(), &counting};
    Table table;
    table.name = "t";

    for (RecordWriter* const writer : all)
        EXPECT_EQ(writer->write(RunRecord{}, {table, table}), WriteStatus::Written);
    EXPECT_EQ(counting.seen, 2U);
    EXPECT_EQ(out.str().rfind("== t ==\n", 0U), 0U);
}
