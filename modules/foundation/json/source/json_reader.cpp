/**************************************************************************/
/* json_reader.cpp                                                        */
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
 * @file json_reader.cpp
 * @brief Reads JSON by recursive descent, keeping 64-bit integers and refusing with a place.
 * @author Mustafa Garip
 * @see modules/foundation/json/README.md
 */

#include <SushiProfiler/json/json_reader.hpp>

#include <charconv>
#include <cstdint>
#include <system_error>
#include <utility>
#include <vector>

namespace SushiProfiler
{
    namespace
    {
        /// @brief The refusal of a high surrogate that no low surrogate follows.
        constexpr const char* K_LONE_HIGH_SURROGATE =
            "a high surrogate with no low surrogate after it";

        /// @brief Returns whether @p character is JSON whitespace.
        bool is_space(char character) noexcept
        {
            return character == ' ' || character == '\t' || character == '\n' || character == '\r';
        }

        /// @brief Returns whether @p character is a decimal digit.
        bool is_digit(char character) noexcept
        {
            return character >= '0' && character <= '9';
        }

        /// @brief Returns the value of hex digit @p character, or -1.
        int hex_value(char character) noexcept
        {
            if (character >= '0' && character <= '9')
                return character - '0';
            if (character >= 'a' && character <= 'f')
                return character - 'a' + 10;
            if (character >= 'A' && character <= 'F')
                return character - 'A' + 10;
            return -1;
        }

        /// @brief Appends code point @p code as UTF-8 to @p out.
        void append_utf8(std::uint32_t code, std::string& out)
        {
            if (code < 0x80U)
            {
                out.push_back(static_cast<char>(code));
            }
            else if (code < 0x800U)
            {
                out.push_back(static_cast<char>(0xC0U | (code >> 6U)));
                out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
            }
            else if (code < 0x10000U)
            {
                out.push_back(static_cast<char>(0xE0U | (code >> 12U)));
                out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
                out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
            }
            else
            {
                out.push_back(static_cast<char>(0xF0U | (code >> 18U)));
                out.push_back(static_cast<char>(0x80U | ((code >> 12U) & 0x3FU)));
                out.push_back(static_cast<char>(0x80U | ((code >> 6U) & 0x3FU)));
                out.push_back(static_cast<char>(0x80U | (code & 0x3FU)));
            }
        }

        /// @brief Reads one document, remembering the first place it is refused.
        class Parser final
        {
            public:
                /// @brief Reads from @p text, which outlives the parser.
                explicit Parser(std::string_view text) : text_(text)
                {

                }

                /// @brief Reads the whole document.
                JsonReadResult run()
                {
                    JsonReadResult result;
                    JsonValue value;
                    skip_space();
                    if (parse_value(value, 0U))
                    {
                        skip_space();
                        if (pos_ == text_.size())
                            result.value = std::move(value);
                        else
                            fail("unexpected text after the value");
                    }
                    if (!result.ok())
                        result.error = error_at(failed_at_, message_);
                    return result;
                }

            private:
                /// @brief Records @p message at @p at, keeping the first failure; returns false.
                bool fail(std::string message, std::size_t at)
                {
                    if (message_.empty())
                    {
                        message_ = std::move(message);
                        failed_at_ = at;
                    }
                    return false;
                }

                /// @brief Records @p message at the current place; returns false.
                bool fail(std::string message)
                {
                    return fail(std::move(message), pos_);
                }

                /// @brief Returns the line and column of byte @p at.
                [[nodiscard]] JsonError error_at(std::size_t at, const std::string& message) const
                {
                    JsonError error;
                    error.line = 1U;
                    error.column = 1U;
                    for (std::size_t index = 0U; index < at && index < text_.size(); ++index)
                    {
                        if (text_[index] == '\n')
                        {
                            ++error.line;
                            error.column = 1U;
                        }
                        else
                        {
                            ++error.column;
                        }
                    }
                    error.message = message;
                    return error;
                }

                /// @brief Moves past whitespace.
                void skip_space() noexcept
                {
                    while (pos_ < text_.size() && is_space(text_[pos_]))
                        ++pos_;
                }

                /// @brief Returns the current character, or a null character at the end.
                [[nodiscard]] char peek() const noexcept
                {
                    return pos_ < text_.size() ? text_[pos_] : '\0';
                }

                /// @brief Reads the value at the current place into @p out.
                bool parse_value(JsonValue& out, std::size_t depth)
                {
                    if (pos_ >= text_.size())
                        return fail("a value was expected, the text ended");
                    const char character = text_[pos_];
                    if (character == '{' || character == '[')
                    {
                        if (depth >= K_JSON_MAX_DEPTH)
                            return fail("nesting deeper than 128");
                        return character == '{' ? parse_object(out, depth + 1U)
                                                : parse_array(out, depth + 1U);
                    }
                    if (character == '"')
                    {
                        std::string text;
                        if (!parse_string(text))
                            return false;
                        out = JsonValue::string(std::move(text));
                        return true;
                    }
                    if (character == '-' || is_digit(character))
                        return parse_number(out);
                    if (match("true"))
                        out = JsonValue::boolean(true);
                    else if (match("false"))
                        out = JsonValue::boolean(false);
                    else if (match("null"))
                        out = JsonValue();
                    else
                        return fail("a value was expected");
                    return true;
                }

                /// @brief Moves past @p word when the text continues with it.
                bool match(std::string_view word) noexcept
                {
                    if (text_.substr(pos_, word.size()) != word)
                        return false;
                    pos_ += word.size();
                    return true;
                }

                /// @brief Reads an object; the current character is '{'.
                bool parse_object(JsonValue& out, std::size_t depth)
                {
                    ++pos_;
                    out = JsonValue::object();
                    skip_space();
                    if (peek() == '}')
                    {
                        ++pos_;
                        return true;
                    }
                    while (true)
                    {
                        skip_space();
                        const std::size_t key_at = pos_;
                        if (peek() != '"')
                            return fail("a member name was expected");
                        std::string key;
                        if (!parse_string(key))
                            return false;
                        if (out.find(key) != nullptr)
                            return fail("the key \"" + key + "\" appears twice", key_at);
                        skip_space();
                        if (peek() != ':')
                            return fail("':' was expected after a member name");
                        ++pos_;
                        skip_space();
                        JsonValue member;
                        if (!parse_value(member, depth))
                            return false;
                        out.set(std::move(key), std::move(member));
                        skip_space();
                        if (peek() == ',')
                        {
                            ++pos_;
                            continue;
                        }
                        if (peek() == '}')
                        {
                            ++pos_;
                            return true;
                        }
                        return fail("',' or '}' was expected");
                    }
                }

                /// @brief Reads an array; the current character is '['.
                bool parse_array(JsonValue& out, std::size_t depth)
                {
                    ++pos_;
                    out = JsonValue::array();
                    skip_space();
                    if (peek() == ']')
                    {
                        ++pos_;
                        return true;
                    }
                    while (true)
                    {
                        skip_space();
                        JsonValue element;
                        if (!parse_value(element, depth))
                            return false;
                        out.push_back(std::move(element));
                        skip_space();
                        if (peek() == ',')
                        {
                            ++pos_;
                            continue;
                        }
                        if (peek() == ']')
                        {
                            ++pos_;
                            return true;
                        }
                        return fail("',' or ']' was expected");
                    }
                }

                /// @brief Reads four hex digits into @p out.
                bool parse_hex4(std::uint32_t& out)
                {
                    if (pos_ + 4U > text_.size())
                        return fail("four hex digits were expected");
                    out = 0U;
                    for (std::size_t index = 0U; index < 4U; ++index)
                    {
                        const int digit = hex_value(text_[pos_ + index]);
                        if (digit < 0)
                            return fail("four hex digits were expected", pos_ + index);
                        out = (out << 4U) | static_cast<std::uint32_t>(digit);
                    }
                    pos_ += 4U;
                    return true;
                }

                /// @brief Reads the escape after a backslash into @p out.
                bool parse_escape(std::string& out)
                {
                    const std::size_t escape_at = pos_ - 1U;
                    const char character = peek();
                    ++pos_;
                    switch (character)
                    {
                        case '"': out.push_back('"'); return true;
                        case '\\': out.push_back('\\'); return true;
                        case '/': out.push_back('/'); return true;
                        case 'b': out.push_back('\b'); return true;
                        case 'f': out.push_back('\f'); return true;
                        case 'n': out.push_back('\n'); return true;
                        case 'r': out.push_back('\r'); return true;
                        case 't': out.push_back('\t'); return true;
                        case 'u': break;
                        default: return fail("an unknown escape", escape_at);
                    }
                    std::uint32_t code = 0U;
                    if (!parse_hex4(code))
                        return false;
                    if (code >= 0xDC00U && code <= 0xDFFFU)
                        return fail("a low surrogate with no high surrogate before it", escape_at);
                    if (code >= 0xD800U && code <= 0xDBFFU)
                    {
                        if (!match("\\u"))
                            return fail(K_LONE_HIGH_SURROGATE, escape_at);
                        std::uint32_t low = 0U;
                        if (!parse_hex4(low))
                            return false;
                        if (low < 0xDC00U || low > 0xDFFFU)
                            return fail(K_LONE_HIGH_SURROGATE, escape_at);
                        code = 0x10000U + ((code - 0xD800U) << 10U) + (low - 0xDC00U);
                    }
                    append_utf8(code, out);
                    return true;
                }

                /// @brief Reads a string; the current character is '"'.
                bool parse_string(std::string& out)
                {
                    ++pos_;
                    while (pos_ < text_.size())
                    {
                        const char character = text_[pos_];
                        if (character == '"')
                        {
                            ++pos_;
                            return true;
                        }
                        if (static_cast<unsigned char>(character) < 0x20U)
                            return fail("a control character inside a string");
                        ++pos_;
                        if (character != '\\')
                            out.push_back(character);
                        else if (!parse_escape(out))
                            return false;
                    }
                    return fail("the text ended inside a string");
                }

                /// @brief Moves past one or more digits; false when there is none.
                bool skip_digits() noexcept
                {
                    const std::size_t start = pos_;
                    while (pos_ < text_.size() && is_digit(text_[pos_]))
                        ++pos_;
                    return pos_ > start;
                }

                /// @brief Reads a number, whole numbers as 64-bit integers when they fit.
                bool parse_number(JsonValue& out)
                {
                    const std::size_t start = pos_;
                    if (peek() == '-')
                        ++pos_;
                    if (peek() == '0')
                        ++pos_;
                    else if (!skip_digits())
                        return fail("a digit was expected");
                    bool whole = true;
                    if (peek() == '.')
                    {
                        ++pos_;
                        whole = false;
                        if (!skip_digits())
                            return fail("a digit was expected after '.'");
                    }
                    if (peek() == 'e' || peek() == 'E')
                    {
                        ++pos_;
                        whole = false;
                        if (peek() == '+' || peek() == '-')
                            ++pos_;
                        if (!skip_digits())
                            return fail("a digit was expected in the exponent");
                    }
                    const char* const first = text_.data() + start;
                    const char* const last = text_.data() + pos_;
                    if (whole)
                    {
                        std::int64_t integer = 0;
                        if (std::from_chars(first, last, integer).ec == std::errc())
                        {
                            out = JsonValue::integer(integer);
                            return true;
                        }
                        std::uint64_t large = 0U;
                        if (*first != '-' && std::from_chars(first, last, large).ec == std::errc())
                        {
                            out = JsonValue::unsigned_integer(large);
                            return true;
                        }
                    }
                    double real = 0.0;
                    if (std::from_chars(first, last, real).ec != std::errc())
                        return fail("a number out of range", start);
                    out = JsonValue::real(real);
                    return true;
                }

                /// @brief The document.
                std::string_view text_;

                /// @brief The current byte.
                std::size_t pos_ = 0U;

                /// @brief The byte of the first failure.
                std::size_t failed_at_ = 0U;

                /// @brief The first failure's message, empty while none.
                std::string message_;
        };
    } // namespace

    JsonReadResult read_json(std::string_view text)
    {
        return Parser(text).run();
    }
} // namespace SushiProfiler
