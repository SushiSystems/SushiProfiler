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
 * @file test_json_value.cpp
 * @brief Tests the JSON value: its kinds, its accessors, member lookup and equality.
 * @author Mustafa Garip
 * @see tests/foundation/json/README.md
 */

#include <SushiProfiler/json/json_value.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

using SushiProfiler::JsonKind;
using SushiProfiler::JsonValue;

TEST(Small_JsonValue, EachFactoryMakesItsKind)
{
    EXPECT_EQ(JsonValue().kind(), JsonKind::Null);
    EXPECT_EQ(JsonValue::boolean(true).kind(), JsonKind::Bool);
    EXPECT_EQ(JsonValue::integer(-3).kind(), JsonKind::Integer);
    EXPECT_EQ(JsonValue::unsigned_integer(3U).kind(), JsonKind::Unsigned);
    EXPECT_EQ(JsonValue::real(0.5).kind(), JsonKind::Real);
    EXPECT_EQ(JsonValue::string("x").kind(), JsonKind::String);
    EXPECT_EQ(JsonValue::array().kind(), JsonKind::Array);
    EXPECT_EQ(JsonValue::object().kind(), JsonKind::Object);
}

TEST(Small_JsonValue, AccessorsAnswerOnlyForTheirKind)
{
    const JsonValue text = JsonValue::string("frame");

    ASSERT_NE(text.as_string(), nullptr);
    EXPECT_EQ(*text.as_string(), "frame");
    EXPECT_EQ(text.as_bool(), nullptr);
    EXPECT_EQ(text.as_array(), nullptr);
    EXPECT_EQ(text.as_object(), nullptr);
}

TEST(Small_JsonValue, NumbersConvertWhenTheyFit)
{
    std::uint64_t whole = 0U;
    double real = 0.0;

    EXPECT_TRUE(JsonValue::integer(7).to_uint64(whole));
    EXPECT_EQ(whole, 7U);
    EXPECT_FALSE(JsonValue::integer(-1).to_uint64(whole));
    const std::uint64_t largest = std::numeric_limits<std::uint64_t>::max();
    EXPECT_TRUE(JsonValue::unsigned_integer(largest).to_uint64(whole));
    EXPECT_EQ(whole, largest);
    EXPECT_FALSE(JsonValue::real(1.0).to_uint64(whole));
    EXPECT_TRUE(JsonValue::integer(-2).to_double(real));
    EXPECT_EQ(real, -2.0);
    EXPECT_FALSE(JsonValue::string("2").to_double(real));
}

TEST(Small_JsonValue, AnObjectKeepsItsMembersInOrderAndFindsThem)
{
    JsonValue object = JsonValue::object();
    object.set("b", JsonValue::integer(1));
    object.set("a", JsonValue::integer(2));
    object.set("b", JsonValue::integer(3));

    ASSERT_EQ(object.as_object()->size(), 2U);
    EXPECT_EQ(object.as_object()->front().key, "b");
    ASSERT_NE(object.find("b"), nullptr);
    EXPECT_EQ(*object.find("b"), JsonValue::integer(3));
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

TEST(Small_JsonValue, EqualityComparesKindAndContent)
{
    EXPECT_EQ(JsonValue::integer(1), JsonValue::integer(1));
    EXPECT_NE(JsonValue::integer(1), JsonValue::unsigned_integer(1U));
    EXPECT_NE(JsonValue::integer(1), JsonValue::real(1.0));
    EXPECT_NE(JsonValue::string("a"), JsonValue::string("b"));
}
