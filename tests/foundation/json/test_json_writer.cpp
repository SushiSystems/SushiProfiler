/**************************************************************************/
/* test_json_writer.cpp                                                   */
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
 * @file test_json_writer.cpp
 * @brief Tests the JSON writer: layout, escapes, reals that read back as reals, and round trips.
 * @author Mustafa Garip
 * @see tests/foundation/json/README.md
 */

#include <SushiProfiler/json/json_writer.hpp>

#include <SushiProfiler/json/json_reader.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

using SushiProfiler::JsonLayout;
using SushiProfiler::JsonValue;
using SushiProfiler::JsonWriter;
using SushiProfiler::read_json;

TEST(Small_JsonWriter, WritesACompactDocument)
{
    JsonWriter writer(false);
    writer.begin_object();
    writer.key("a");
    writer.integer(1);
    writer.key("b");
    writer.begin_array();
    writer.boolean(true);
    writer.null();
    writer.end_array();
    writer.end_object();

    EXPECT_EQ(writer.text(), R"({"a":1,"b":[true,null]})");
}

TEST(Small_JsonWriter, IndentsBlocksAndKeepsInlineArraysOnOneLine)
{
    JsonWriter writer(true);
    writer.begin_object();
    writer.key("columns");
    writer.begin_array(JsonLayout::Inline);
    writer.integer(1);
    writer.integer(2);
    writer.end_array();
    writer.key("empty");
    writer.begin_object();
    writer.end_object();
    writer.end_object();

    EXPECT_EQ(writer.text(), "{\n  \"columns\": [1, 2],\n  \"empty\": {}\n}");
}

TEST(Small_JsonWriter, EscapesControlCharactersAndQuotes)
{
    JsonWriter writer(false);
    writer.string("a\"b\\\n\x01");

    EXPECT_EQ(writer.text(), "\"a\\\"b\\\\\\n\\u0001\"");
}

TEST(Small_JsonWriter, AWholeRealStaysAReal)
{
    JsonWriter writer(false);
    writer.begin_array();
    writer.real(1.0);
    writer.real(-0.0);
    writer.real(0.1);
    writer.real(1e300);
    writer.end_array();

    EXPECT_EQ(writer.text(), "[1.0,-0.0,0.1,1e+300]");
}

TEST(Small_JsonWriter, ANonFiniteRealIsNull)
{
    JsonWriter writer(false);
    writer.real(std::numeric_limits<double>::quiet_NaN());

    EXPECT_EQ(writer.text(), "null");
}

TEST(Small_JsonWriter, AValueRoundTrips)
{
    const std::string text = "{\"i\":-9223372036854775808,\"u\":18446744073709551615,\"r\":2.5,"
                             "\"s\":\"\\u00e9\",\"a\":[[],{}]}";
    const auto first = read_json(text);
    ASSERT_TRUE(first.ok());
    JsonWriter writer(true);
    writer.value(*first.value);
    const auto second = read_json(writer.text());

    ASSERT_TRUE(second.ok()) << second.error.message;
    EXPECT_EQ(*second.value, *first.value);
}
