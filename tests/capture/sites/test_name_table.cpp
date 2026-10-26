/**************************************************************************/
/* test_name_table.cpp                                                    */
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
 * @file test_name_table.cpp
 * @brief Tests that the name table gives one id per text and 0 for empty text.
 * @author Mustafa Garip
 * @see tests/capture/sites/README.md
 */

#include <SushiProfiler/sites/name_table.hpp>

#include <gtest/gtest.h>

#include <string>

using SushiProfiler::NameID;
using SushiProfiler::NameTable;

TEST(Small_NameTable, SameTextSameId)
{
    NameTable names;
    std::string text = "train.step";

    const NameID first = names.intern(text);
    text = "something else";

    EXPECT_NE(first, 0U);
    EXPECT_EQ(names.intern("train.step"), first);
    EXPECT_NE(names.intern(text), first);
    EXPECT_EQ(names.text(first), "train.step");
    EXPECT_EQ(names.size(), 2U);
}

TEST(Small_NameTable, EmptyTextIsZero)
{
    NameTable names;

    EXPECT_EQ(names.intern(""), 0U);
    EXPECT_EQ(names.text(0U), "");
    EXPECT_EQ(names.size(), 0U);
}

TEST(Small_NameTable, AnUnknownIdReadsEmpty)
{
    NameTable names;

    EXPECT_EQ(names.text(7U), "");
}
