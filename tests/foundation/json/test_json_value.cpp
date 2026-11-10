/**************************************************************************/
/* test_json_value.cpp                                                    */
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
 * @file test_json_value.cpp
 * @brief Tests the JSON value: its kinds, its accessors, member lookup and equality.
 * @author Mustafa Garip
 * @see tests/foundation/json/README.md
 */

#include <SushiProfiler/json/json_value.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <limits>
#include <tuple>
#include <utility>

using SushiProfiler::JsonKind;
using SushiProfiler::JsonValue;

namespace
{
    /// @brief Returns an array holding @p elements in order.
    JsonValue array_of(std::initializer_list<JsonValue> elements)
    {
        JsonValue array = JsonValue::array();
        for (const JsonValue& element : elements)
            array.push_back(element);
        return array;
    }

    /// @brief Returns an object with the member "a" holding @p first and the member "b" holding @p second.
    JsonValue pair_object(JsonValue first, JsonValue second)
    {
        JsonValue object = JsonValue::object();
        object.set("a", std::move(first));
        object.set("b", std::move(second));
        return object;
    }

    using KindRow = std::pair<JsonValue, JsonKind>;
    using EqualityRow = std::tuple<JsonValue, JsonValue, bool>;
} // namespace

class Small_JsonValueKind : public ::testing::TestWithParam<KindRow>
{
};

TEST_P(Small_JsonValueKind, AFactoryMakesTheKindItNames)
{
    EXPECT_EQ(GetParam().first.kind(), GetParam().second);
}

INSTANTIATE_TEST_SUITE_P(
    Small_Factories, Small_JsonValueKind,
    ::testing::Values(KindRow{JsonValue(), JsonKind::Null},
                      KindRow{JsonValue::boolean(true), JsonKind::Bool},
                      KindRow{JsonValue::integer(-3), JsonKind::Integer},
                      KindRow{JsonValue::unsigned_integer(3U), JsonKind::Unsigned},
                      KindRow{JsonValue::real(0.5), JsonKind::Real},
                      KindRow{JsonValue::string("x"), JsonKind::String},
                      KindRow{JsonValue::array(), JsonKind::Array},
                      KindRow{JsonValue::object(), JsonKind::Object}));

TEST(Small_JsonValue, AStringAnswersAsAString)
{
    const JsonValue text = JsonValue::string("frame");

    ASSERT_NE(text.as_string(), nullptr);
    EXPECT_EQ(*text.as_string(), "frame");
}

TEST(Small_JsonValue, AStringDoesNotAnswerAsAnotherKind)
{
    const JsonValue text = JsonValue::string("frame");

    EXPECT_EQ(text.as_bool(), nullptr);
    EXPECT_EQ(text.as_array(), nullptr);
    EXPECT_EQ(text.as_object(), nullptr);
}

class Small_JsonValueToUnsigned : public ::testing::TestWithParam<std::pair<JsonValue, std::uint64_t>>
{
};

TEST_P(Small_JsonValueToUnsigned, ANumberThatFitsConvertsExactly)
{
    std::uint64_t whole = 0U;

    EXPECT_TRUE(GetParam().first.to_uint64(whole));
    EXPECT_EQ(whole, GetParam().second);
}

INSTANTIATE_TEST_SUITE_P(
    Small_Fits, Small_JsonValueToUnsigned,
    ::testing::Values(std::pair<JsonValue, std::uint64_t>{JsonValue::integer(7), 7U},
                      std::pair<JsonValue, std::uint64_t>{
                          JsonValue::unsigned_integer(std::numeric_limits<std::uint64_t>::max()),
                          std::numeric_limits<std::uint64_t>::max()}));

class Small_JsonValueNotUnsigned : public ::testing::TestWithParam<JsonValue>
{
};

TEST_P(Small_JsonValueNotUnsigned, AValueThatDoesNotFitIsRefused)
{
    std::uint64_t whole = 0U;

    EXPECT_FALSE(GetParam().to_uint64(whole));
}

INSTANTIATE_TEST_SUITE_P(Small_Refusals, Small_JsonValueNotUnsigned,
                         ::testing::Values(JsonValue::integer(-1), JsonValue::real(1.0)));

TEST(Small_JsonValue, AnIntegerConvertsToADouble)
{
    double real = 0.0;

    EXPECT_TRUE(JsonValue::integer(-2).to_double(real));
    EXPECT_EQ(real, -2.0);
}

TEST(Small_JsonValue, AStringDoesNotConvertToADouble)
{
    double real = 0.0;

    EXPECT_FALSE(JsonValue::string("2").to_double(real));
}

TEST(Small_JsonValue, AnObjectKeepsItsMembersInTheOrderTheyWereSet)
{
    const JsonValue object = pair_object(JsonValue::integer(1), JsonValue::integer(2));

    ASSERT_EQ(object.as_object()->size(), 2U);
    EXPECT_EQ(object.as_object()->front().key, "a");
    EXPECT_EQ(object.as_object()->back().key, "b");
}

TEST(Small_JsonValue, SettingAKnownKeyReplacesItInPlace)
{
    JsonValue object = pair_object(JsonValue::integer(1), JsonValue::integer(2));

    object.set("a", JsonValue::integer(3));

    ASSERT_EQ(object.as_object()->size(), 2U);
    EXPECT_EQ(object.as_object()->front().key, "a");
    EXPECT_EQ(object.as_object()->front().value, JsonValue::integer(3));
}

TEST(Small_JsonValue, FindReturnsTheMemberNamed)
{
    const JsonValue object = pair_object(JsonValue::integer(1), JsonValue::integer(2));

    ASSERT_NE(object.find("b"), nullptr);
    EXPECT_EQ(*object.find("b"), JsonValue::integer(2));
}

TEST(Small_JsonValue, FindReturnsNullForAKeyNotSet)
{
    const JsonValue object = pair_object(JsonValue::integer(1), JsonValue::integer(2));

    EXPECT_EQ(object.find("c"), nullptr);
}

TEST(Small_JsonValue, AnArrayAppends)
{
    JsonValue array = JsonValue::array();
    array.push_back(JsonValue::boolean(false));
    array.push_back(JsonValue());

    ASSERT_EQ(array.as_array()->size(), 2U);
    EXPECT_EQ((*array.as_array())[1], JsonValue());
}

class Small_JsonValueEquality : public ::testing::TestWithParam<EqualityRow>
{
};

TEST_P(Small_JsonValueEquality, ValuesAreEqualWhenKindAndContentAre)
{
    EXPECT_EQ(std::get<0>(GetParam()) == std::get<1>(GetParam()), std::get<2>(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
    Small_Pairs, Small_JsonValueEquality,
    ::testing::Values(
        EqualityRow{JsonValue::integer(1), JsonValue::integer(1), true},
        EqualityRow{JsonValue::integer(1), JsonValue::unsigned_integer(1U), false},
        EqualityRow{JsonValue::integer(1), JsonValue::real(1.0), false},
        EqualityRow{JsonValue::string("a"), JsonValue::string("b"), false},
        EqualityRow{array_of({JsonValue::integer(1), array_of({JsonValue::integer(2)})}),
                    array_of({JsonValue::integer(1), array_of({JsonValue::integer(2)})}), true},
        EqualityRow{array_of({JsonValue::integer(1), array_of({JsonValue::integer(2)})}),
                    array_of({JsonValue::integer(1), array_of({JsonValue::integer(3)})}), false},
        EqualityRow{pair_object(JsonValue::integer(1), array_of({JsonValue::integer(2)})),
                    pair_object(JsonValue::integer(1), array_of({JsonValue::integer(2)})), true},
        EqualityRow{pair_object(JsonValue::integer(1), JsonValue::integer(2)),
                    pair_object(JsonValue::integer(1), JsonValue::integer(3)), false}));
