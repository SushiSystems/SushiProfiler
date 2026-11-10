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

TEST(Small_JsonFieldReader, ReadsAWholeNumberField)
{
    const JsonValue value = parsed(R"({"n": 7})");
    JsonFieldReader reader;
    std::uint8_t whole = 0U;

    ASSERT_TRUE(reader.object(value, {"n"}));
    EXPECT_TRUE(reader.whole(value, "n", whole));
    EXPECT_EQ(whole, 7U);
}

TEST(Small_JsonFieldReader, ReadsATextField)
{
    const JsonValue value = parsed(R"({"s": "x"})");
    JsonFieldReader reader;
    std::string text;

    ASSERT_TRUE(reader.object(value, {"s"}));
    EXPECT_TRUE(reader.text(value, "s", text));
    EXPECT_EQ(text, "x");
}

TEST(Small_JsonFieldReader, ReadsAFlagField)
{
    const JsonValue value = parsed(R"({"b": true})");
    JsonFieldReader reader;
    bool flag = false;

    ASSERT_TRUE(reader.object(value, {"b"}));
    EXPECT_TRUE(reader.flag(value, "b", flag));
    EXPECT_TRUE(flag);
}

TEST(Small_JsonFieldReader, ReadsARealFieldWrittenAsInfinity)
{
    const JsonValue value = parsed(R"({"r": "Infinity"})");
    JsonFieldReader reader;
    double real = 0.0;

    ASSERT_TRUE(reader.object(value, {"r"}));
    EXPECT_TRUE(reader.real(value, "r", real));
    EXPECT_EQ(real, HUGE_VAL);
}

TEST(Small_JsonFieldReader, ReadsAColumnOfWholeNumbers)
{
    const JsonValue value = parsed(R"({"c": [1, 2]})");
    JsonFieldReader reader;
    std::vector<std::uint32_t> column;

    ASSERT_TRUE(reader.object(value, {"c"}));
    EXPECT_TRUE(reader.wholes(value, "c", column));
    EXPECT_EQ(column, (std::vector<std::uint32_t>{1U, 2U}));
}

TEST(Small_JsonFieldReader, AStrictObjectNeedsEveryKey)
{
    JsonFieldReader reader("config");

    EXPECT_FALSE(reader.object(parsed(R"({"a": 1})"), {"a", "b"}));
    EXPECT_EQ(reader.error(), "config: missing key \"b\"");
}

TEST(Small_JsonFieldReader, AStrictObjectRefusesAKeyItDoesNotName)
{
    JsonFieldReader reader("config");

    EXPECT_FALSE(reader.object(parsed(R"({"a": 1, "z": 2})"), {"a"}));
    EXPECT_EQ(reader.error(), "config: unknown key \"z\"");
}

TEST(Small_JsonFieldReader, AnOptionalObjectAcceptsMissingKeys)
{
    JsonFieldReader reader;

    EXPECT_TRUE(reader.object_within(parsed(R"({"a": 1})"), {"a", "b"}));
}

TEST(Small_JsonFieldReader, HasReportsWhetherAKeyIsPresent)
{
    const JsonValue value = parsed(R"({"a": 1})");

    EXPECT_TRUE(JsonFieldReader::has(value, "a"));
    EXPECT_FALSE(JsonFieldReader::has(value, "b"));
}

TEST(Small_JsonFieldReader, AnOptionalObjectRefusesAKeyItDoesNotName)
{
    JsonFieldReader reader;

    EXPECT_FALSE(reader.object_within(parsed(R"({"c": 1})"), {"a", "b"}));
    EXPECT_EQ(reader.error(), "document: unknown key \"c\"");
}

TEST(Small_JsonFieldReader, ARefusalNamesItsPath)
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
}

TEST(Small_JsonFieldReader, TheFirstRefusalStays)
{
    JsonFieldReader reader;
    EXPECT_FALSE(reader.fail("the first refusal"));

    EXPECT_FALSE(reader.fail("a later refusal"));

    EXPECT_EQ(reader.error(), "document: the first refusal");
}
