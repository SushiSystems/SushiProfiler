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
#include <utility>

using SushiProfiler::JsonLayout;
using SushiProfiler::JsonValue;
using SushiProfiler::JsonWriter;
using SushiProfiler::read_json;

namespace
{
    using RealRow = std::pair<double, const char*>;
    using EscapeRow = std::pair<const char*, const char*>;
} // namespace

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

TEST(Small_JsonWriter, TheIndentedWriterPutsEachMemberOnItsOwnLine)
{
    JsonWriter writer(true);
    writer.begin_object();
    writer.key("a");
    writer.integer(1);
    writer.key("empty");
    writer.begin_object();
    writer.end_object();
    writer.end_object();

    EXPECT_EQ(writer.text(), "{\n  \"a\": 1,\n  \"empty\": {}\n}");
}

TEST(Small_JsonWriter, AnInlineArrayStaysOnOneLineInTheIndentedWriter)
{
    JsonWriter writer(true);
    writer.begin_object();
    writer.key("columns");
    writer.begin_array(JsonLayout::Inline);
    writer.integer(1);
    writer.integer(2);
    writer.end_array();
    writer.end_object();

    EXPECT_EQ(writer.text(), "{\n  \"columns\": [1, 2]\n}");
}

class Small_JsonWriterEscape : public ::testing::TestWithParam<EscapeRow>
{
};

TEST_P(Small_JsonWriterEscape, AStringWritesItsEscapedForm)
{
    JsonWriter writer(false);
    writer.string(GetParam().first);

    EXPECT_EQ(writer.text(), GetParam().second);
}

INSTANTIATE_TEST_SUITE_P(Small_Rules, Small_JsonWriterEscape,
                         ::testing::Values(EscapeRow{"a\"b", "\"a\\\"b\""},
                                           EscapeRow{"a\\b", "\"a\\\\b\""},
                                           EscapeRow{"a\nb", "\"a\\nb\""},
                                           EscapeRow{"a\x01", "\"a\\u0001\""}));

class Small_JsonWriterReal : public ::testing::TestWithParam<RealRow>
{
};

TEST_P(Small_JsonWriterReal, ARealWritesInTheShortestFormThatReadsBackAsAReal)
{
    JsonWriter writer(false);
    writer.real(GetParam().first);

    EXPECT_EQ(writer.text(), GetParam().second);
}

INSTANTIATE_TEST_SUITE_P(Small_Values, Small_JsonWriterReal,
                         ::testing::Values(RealRow{1.0, "1.0"}, RealRow{-0.0, "-0.0"},
                                           RealRow{0.1, "0.1"}, RealRow{1e300, "1e+300"}));

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
