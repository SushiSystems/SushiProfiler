/**************************************************************************/
/* test_json_reader.cpp                                                   */
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
 * @file test_json_reader.cpp
 * @brief Tests the JSON reader: every kind, exact integers, escapes, and errors with a place.
 * @author Mustafa Garip
 * @see tests/foundation/json/README.md
 */

#include <SushiProfiler/json/json_reader.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

using SushiProfiler::JsonKind;
using SushiProfiler::JsonReadResult;
using SushiProfiler::JsonValue;
using SushiProfiler::read_json;

TEST(Small_JsonReader, ReadsEveryKind)
{
    const JsonReadResult result =
        read_json(R"({"n": null, "t": true, "f": false, "i": -12, "r": 2.5, "s": "x",)"
                  R"( "a": [1, [2]]})");

    ASSERT_TRUE(result.ok()) << result.error.message;
    const JsonValue& value = *result.value;
    EXPECT_EQ(value.find("n")->kind(), JsonKind::Null);
    EXPECT_EQ(*value.find("t")->as_bool(), true);
    EXPECT_EQ(*value.find("f")->as_bool(), false);
    EXPECT_EQ(*value.find("i")->as_integer(), -12);
    EXPECT_EQ(*value.find("r")->as_real(), 2.5);
    EXPECT_EQ(*value.find("s")->as_string(), "x");
    EXPECT_EQ(value.find("a")->as_array()->size(), 2U);
}

TEST(Small_JsonReader, KeepsSixtyFourBitIntegersExact)
{
    const JsonReadResult largest = read_json("18446744073709551615");
    const JsonReadResult smallest = read_json("-9223372036854775808");
    const JsonReadResult beyond = read_json("18446744073709551616");

    ASSERT_TRUE(largest.ok() && smallest.ok() && beyond.ok());
    EXPECT_EQ(*largest.value->as_unsigned(), std::numeric_limits<std::uint64_t>::max());
    EXPECT_EQ(*smallest.value->as_integer(), std::numeric_limits<std::int64_t>::min());
    EXPECT_EQ(beyond.value->kind(), JsonKind::Real);
}

TEST(Small_JsonReader, ReadsRealsWithExponentsAndSignedZero)
{
    const JsonReadResult zero = read_json("-0.0");
    const JsonReadResult large = read_json("1e300");

    ASSERT_TRUE(zero.ok() && large.ok());
    EXPECT_TRUE(std::signbit(*zero.value->as_real()));
    EXPECT_EQ(*large.value->as_real(), 1e300);
}

TEST(Small_JsonReader, DecodesEscapesAndSurrogatePairs)
{
    const JsonReadResult result =
        read_json("\"a\\\"b\\\\c\\/\\n\\t\\u00e9\\ud83d\\ude00\"");

    ASSERT_TRUE(result.ok()) << result.error.message;
    EXPECT_EQ(*result.value->as_string(), "a\"b\\c/\n\t\xC3\xA9\xF0\x9F\x98\x80");
}

TEST(Small_JsonReader, AnErrorNamesItsLineAndColumn)
{
    const JsonReadResult result = read_json("{\n  \"a\": 1,\n  \"b\": tru\n}");

    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error.line, 3U);
    EXPECT_EQ(result.error.column, 8U);
}

TEST(Small_JsonReader, RefusesWhatJsonDoesNotAllow)
{
    EXPECT_FALSE(read_json("").ok());
    EXPECT_FALSE(read_json("[1,]").ok());
    EXPECT_FALSE(read_json("{\"a\" 1}").ok());
    EXPECT_FALSE(read_json("01").ok());
    EXPECT_FALSE(read_json("1.").ok());
    EXPECT_FALSE(read_json("\"\x01\"").ok());
    EXPECT_FALSE(read_json("\"\\ud83d\"").ok());
    EXPECT_FALSE(read_json("1 2").ok());
    EXPECT_FALSE(read_json("NaN").ok());
}

TEST(Small_JsonReader, RefusesNestingDeeperThanTheLimit)
{
    const std::string deep(SushiProfiler::K_JSON_MAX_DEPTH + 1U, '[');
    const std::string allowed = std::string(SushiProfiler::K_JSON_MAX_DEPTH, '[') +
                                std::string(SushiProfiler::K_JSON_MAX_DEPTH, ']');

    EXPECT_FALSE(read_json(deep).ok());
    EXPECT_TRUE(read_json(allowed).ok());
}

TEST(Small_JsonReader, ADuplicateKeyIsRefused)
{
    const JsonReadResult result = read_json(R"({"a": 1, "a": 2})");

    ASSERT_FALSE(result.ok());
    EXPECT_NE(result.error.message.find("\"a\""), std::string::npos);
}
