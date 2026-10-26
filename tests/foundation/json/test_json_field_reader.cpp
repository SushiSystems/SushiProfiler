/**************************************************************************/
/* test_json_field_reader.cpp                                             */
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
 * @file test_json_field_reader.cpp
 * @brief Tests strict and optional objects, kinds, ranges and the path in each refusal.
 * @author Mustafa Garip
 * @see tests/foundation/json/README.md
 */

#include <SushiProfiler/json/json_field_reader.hpp>

#include <gtest/gtest.h>

#include <cmath>

#include <SushiProfiler/json/json_reader.hpp>

using SushiProfiler::JsonFieldReader;
using SushiProfiler::JsonValue;

namespace
{
    /// @brief Returns the value @p text holds.
    JsonValue parsed(const char* text)
    {
        return *SushiProfiler::read_json(text).value;
    }
} // namespace

TEST(Small_JsonFieldReader, ReadsFieldsOfEveryKind)
{
    const JsonValue value =
        parsed(R"({"n": 7, "s": "x", "b": true, "r": "Infinity", "c": [1, 2]})");
    JsonFieldReader reader;
    std::uint8_t whole = 0U;
    std::string text;
    bool flag = false;
    double real = 0.0;
    std::vector<std::uint32_t> column;

    ASSERT_TRUE(reader.object(value, {"n", "s", "b", "r", "c"}));
    EXPECT_TRUE(reader.whole(value, "n", whole));
    EXPECT_TRUE(reader.text(value, "s", text));
    EXPECT_TRUE(reader.flag(value, "b", flag));
    EXPECT_TRUE(reader.real(value, "r", real));
    EXPECT_TRUE(reader.wholes(value, "c", column));
    EXPECT_EQ(whole, 7U);
    EXPECT_EQ(text, "x");
    EXPECT_TRUE(flag);
    EXPECT_EQ(real, HUGE_VAL);
    EXPECT_EQ(column, (std::vector<std::uint32_t>{1U, 2U}));
    EXPECT_TRUE(reader.error().empty());
}

TEST(Small_JsonFieldReader, AStrictObjectNeedsEveryKeyAndNoOther)
{
    JsonFieldReader missing("config");
    EXPECT_FALSE(missing.object(parsed(R"({"a": 1})"), {"a", "b"}));
    EXPECT_EQ(missing.error(), "config: missing key \"b\"");

    JsonFieldReader unknown("config");
    EXPECT_FALSE(unknown.object(parsed(R"({"a": 1, "z": 2})"), {"a"}));
    EXPECT_EQ(unknown.error(), "config: unknown key \"z\"");
}

TEST(Small_JsonFieldReader, AnOptionalObjectAcceptsMissingKeys)
{
    const JsonValue value = parsed(R"({"a": 1})");
    JsonFieldReader reader;

    EXPECT_TRUE(reader.object_within(value, {"a", "b"}));
    EXPECT_TRUE(JsonFieldReader::has(value, "a"));
    EXPECT_FALSE(JsonFieldReader::has(value, "b"));
    EXPECT_FALSE(reader.object_within(parsed(R"({"c": 1})"), {"a", "b"}));
    EXPECT_EQ(reader.error(), "document: unknown key \"c\"");
}

TEST(Small_JsonFieldReader, ARefusalNamesItsPathAndKeepsTheFirst)
{
    const JsonValue value = parsed(R"({"list": [{"n": 300}]})");
    JsonFieldReader reader;
    const auto element = [&reader](const JsonValue& entry)
    {
        std::uint8_t small = 0U;
        return reader.whole(entry, "n", small);
    };

    EXPECT_FALSE(reader.each(value, "list", element));
    EXPECT_EQ(reader.error(), "list[0].n: out of range 0..255");
    EXPECT_FALSE(reader.fail("a later refusal"));
    EXPECT_EQ(reader.error(), "list[0].n: out of range 0..255");
}
