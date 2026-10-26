/**************************************************************************/
/* json_value.hpp                                                         */
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
 * @file json_value.hpp
 * @brief Declares a JSON value that keeps 64-bit integers exact and object members in order.
 * @author Mustafa Garip
 * @see modules/foundation/json/README.md
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace SushiProfiler
{
    /// @brief Names what a JSON value holds.
    enum class JsonKind : std::uint8_t
    {
        Null,     /**< null. */
        Bool,     /**< true or false. */
        Integer,  /**< A whole number from INT64_MIN to INT64_MAX. */
        Unsigned, /**< A whole number above INT64_MAX that fits 64 bits. */
        Real,     /**< A number written with a fraction or an exponent, or too large for 64 bits. */
        String,   /**< Text, held as UTF-8. */
        Array,    /**< An ordered list of values. */
        Object    /**< Members in the order they were set, keys unique. */
    };

    struct JsonMember;

    /// @brief One JSON value of any kind.
    class JsonValue
    {
        public:
            /// @brief Builds null.
            JsonValue() noexcept = default;

            /// @brief Returns true or false.
            [[nodiscard]] static JsonValue boolean(bool value);

            /// @brief Returns a signed whole number.
            [[nodiscard]] static JsonValue integer(std::int64_t value);

            /// @brief Returns an unsigned whole number; one that fits int64 is still Unsigned.
            [[nodiscard]] static JsonValue unsigned_integer(std::uint64_t value);

            /// @brief Returns a real number.
            [[nodiscard]] static JsonValue real(double value);

            /// @brief Returns a string.
            [[nodiscard]] static JsonValue string(std::string value);

            /// @brief Returns an empty array.
            [[nodiscard]] static JsonValue array();

            /// @brief Returns an empty object.
            [[nodiscard]] static JsonValue object();

            /// @brief Returns what the value holds.
            [[nodiscard]] JsonKind kind() const noexcept;

            /// @brief Returns the bool, or null for another kind.
            [[nodiscard]] const bool* as_bool() const noexcept;

            /// @brief Returns the signed whole number, or null for another kind.
            [[nodiscard]] const std::int64_t* as_integer() const noexcept;

            /// @brief Returns the unsigned whole number, or null for another kind.
            [[nodiscard]] const std::uint64_t* as_unsigned() const noexcept;

            /// @brief Returns the real number, or null for another kind.
            [[nodiscard]] const double* as_real() const noexcept;

            /// @brief Returns the string, or null for another kind.
            [[nodiscard]] const std::string* as_string() const noexcept;

            /// @brief Returns the elements, or null for another kind.
            [[nodiscard]] const std::vector<JsonValue>* as_array() const noexcept;

            /// @brief Returns the members, or null for another kind.
            [[nodiscard]] const std::vector<JsonMember>* as_object() const noexcept;

            /// @brief Sets @p out to a whole number that is not negative; false otherwise.
            [[nodiscard]] bool to_uint64(std::uint64_t& out) const noexcept;

            /// @brief Sets @p out to any number as a double; false for another kind.
            [[nodiscard]] bool to_double(double& out) const noexcept;

            /// @brief Returns the member named @p key of an object, or null.
            [[nodiscard]] const JsonValue* find(std::string_view key) const noexcept;

            /// @brief Appends @p element to an array.
            /// @pre The value is an array.
            void push_back(JsonValue element);

            /// @brief Sets member @p key of an object, replacing one already set in its place.
            /// @pre The value is an object.
            void set(std::string key, JsonValue member);

            /// @brief Returns whether both values have one kind and equal content.
            [[nodiscard]] bool operator==(const JsonValue& other) const;

            /// @brief Returns whether the values differ.
            [[nodiscard]] bool operator!=(const JsonValue& other) const;

        private:
            /// @brief The content; the alternatives follow JsonKind's order.
            using Content = std::variant<std::monostate, bool, std::int64_t, std::uint64_t, double,
                                         std::string, std::vector<JsonValue>,
                                         std::vector<JsonMember>>;

            /// @brief Builds a value holding @p content.
            explicit JsonValue(Content content);

            /// @brief The content.
            Content content_;
    };

    /// @brief One member of a JSON object.
    struct JsonMember
    {
        std::string key; /**< The member's name. */
        JsonValue value; /**< The member's value. */

        /// @brief Returns whether key and value are equal.
        [[nodiscard]] bool operator==(const JsonMember& other) const;
    };
} // namespace SushiProfiler
