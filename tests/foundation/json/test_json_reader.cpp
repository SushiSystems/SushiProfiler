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
#include <utility>

using SushiProfiler::JsonKind;
using SushiProfiler::JsonReadResult;
using SushiProfiler::JsonValue;
using SushiProfiler::read_json;

namespace
{
    using KindRow = std::pair<const char*, JsonKind>;

    /// @brief Returns the value @p text holds.
    JsonValue parsed(const char* text)
    {
        return *read_json(text).value;
    }
} // namespace

class Small_JsonReaderKind : public ::testing::TestWithParam<KindRow>
{
};

TEST_P(Small_JsonReaderKind, ADocumentReadsAsTheKindItNames)
{
    const JsonReadResult result = read_json(GetParam().first);

    ASSERT_TRUE(result.ok()) << result.error.message;
    EXPECT_EQ(result.value->kind(), GetParam().second);
}

INSTANTIATE_TEST_SUITE_P(Small_Documents, Small_JsonReaderKind,
                         ::testing::Values(KindRow{"null", JsonKind::Null},
                                           KindRow{"true", JsonKind::Bool},
                                           KindRow{"-12", JsonKind::Integer},
                                           KindRow{"2.5", JsonKind::Real},
                                           KindRow{"\"x\"", JsonKind::String},
                                           KindRow{"[1, [2]]", JsonKind::Array},
                                           KindRow{"{\"a\": 1}", JsonKind::Object}));

TEST(Small_JsonReader, TrueReadsAsTrue)
{
    EXPECT_EQ(*parsed("true").as_bool(), true);
}

TEST(Small_JsonReader, FalseReadsAsFalse)
{
    EXPECT_EQ(*parsed("false").as_bool(), false);
}

TEST(Small_JsonReader, AnIntegerReadsWithItsValue)
{
    EXPECT_EQ(*parsed("-12").as_integer(), -12);
}

TEST(Small_JsonReader, ARealReadsWithItsValue)
{
    EXPECT_EQ(*parsed("2.5").as_real(), 2.5);
}

TEST(Small_JsonReader, AStringReadsWithItsText)
{
    EXPECT_EQ(*parsed("\"x\"").as_string(), "x");
}

TEST(Small_JsonReader, AnArrayReadsItsElementsInOrder)
{
    const JsonValue array = parsed("[1, [2]]");

    ASSERT_EQ(array.as_array()->size(), 2U);
    EXPECT_EQ((*array.as_array())[0], JsonValue::integer(1));
}

TEST(Small_JsonReader, TheLargestUnsignedIntegerReadsExactly)
{
    EXPECT_EQ(*parsed("18446744073709551615").as_unsigned(),
              std::numeric_limits<std::uint64_t>::max());
}

TEST(Small_JsonReader, TheSmallestSignedIntegerReadsExactly)
{
    EXPECT_EQ(*parsed("-9223372036854775808").as_integer(),
              std::numeric_limits<std::int64_t>::min());
}

TEST(Small_JsonReader, ANumberPastSixtyFourBitsReadsAsAReal)
{
    EXPECT_EQ(parsed("18446744073709551616").kind(), JsonKind::Real);
}

TEST(Small_JsonReader, ARealKeepsTheSignOfZero)
{
    EXPECT_TRUE(std::signbit(*parsed("-0.0").as_real()));
}

TEST(Small_JsonReader, ARealKeepsItsExponent)
{
    EXPECT_EQ(*parsed("1e300").as_real(), 1e300);
}

TEST(Small_JsonReader, EscapesDecodeToUtf8)
{
    EXPECT_EQ(*parsed("\"a\\\"b\\\\c\\/\\n\\t\\u00e9\"").as_string(), "a\"b\\c/\n\t\xC3\xA9");
}

TEST(Small_JsonReader, ASurrogatePairDecodesToOneUtf8Character)
{
    EXPECT_EQ(*parsed("\"\\ud83d\\ude00\"").as_string(), "\xF0\x9F\x98\x80");
}

TEST(Small_JsonReader, AnErrorNamesItsLineAndColumn)
{
    const JsonReadResult result = read_json("{\n  \"a\": 1,\n  \"b\": tru\n}");

    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error.line, 3U);
    EXPECT_EQ(result.error.column, 8U);
}

class Small_JsonReaderRefusal : public ::testing::TestWithParam<const char*>
{
};

TEST_P(Small_JsonReaderRefusal, TextJsonDoesNotAllowIsRefused)
{
    EXPECT_FALSE(read_json(GetParam()).ok());
}

INSTANTIATE_TEST_SUITE_P(Small_Inputs, Small_JsonReaderRefusal,
                         ::testing::Values("", "[1,]", "{\"a\" 1}", "01", "1.", "\"\x01\"",
                                           "\"\\ud83d\"", "1 2", "NaN"));

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
