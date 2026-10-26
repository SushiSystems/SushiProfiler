/**************************************************************************/
/* test_table.cpp                                                         */
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
 * @file test_table.cpp
 * @brief Tests cells, their kinds and equality, and finding a column by name.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/table.hpp>

#include <gtest/gtest.h>

using SushiProfiler::Cell;
using SushiProfiler::Table;
using SushiProfiler::Unit;

TEST(Small_Table, ACellHoldsOneKind)
{
    const Cell empty;
    const Cell text = Cell::text("conv");
    const Cell whole = Cell::whole(-3);
    const Cell real = Cell::real(2.5);

    EXPECT_TRUE(empty.empty());
    ASSERT_NE(text.as_text(), nullptr);
    EXPECT_EQ(*text.as_text(), "conv");
    EXPECT_EQ(text.as_whole(), nullptr);
    ASSERT_NE(whole.as_whole(), nullptr);
    EXPECT_EQ(*whole.as_whole(), -3);
    ASSERT_NE(real.as_real(), nullptr);
    EXPECT_EQ(*real.as_real(), 2.5);
}

TEST(Small_Table, CellsOfDifferentKindsDiffer)
{
    EXPECT_EQ(Cell::whole(2), Cell::whole(2));
    EXPECT_NE(Cell::whole(2), Cell::real(2.0));
    EXPECT_NE(Cell::text(""), Cell());
}

TEST(Small_Table, FindsAColumnByName)
{
    Table table;
    table.add_column("group");
    table.add_column("median", Unit::Nanoseconds);

    EXPECT_EQ(table.column("median"), 1U);
    EXPECT_EQ(table.columns[1].unit, Unit::Nanoseconds);
    EXPECT_EQ(table.column("missing"), 2U);
}
