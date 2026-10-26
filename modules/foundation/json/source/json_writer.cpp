/**************************************************************************/
/* json_writer.cpp                                                        */
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
 * @file json_writer.cpp
 * @brief Writes JSON text with separators, indentation, escapes and shortest exact reals.
 * @author Mustafa Garip
 * @see modules/foundation/json/README.md
 */

#include <SushiProfiler/json/json_writer.hpp>

#include <array>
#include <charconv>
#include <cmath>
#include <utility>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns whether @p value is a number, bool, null or string.
        bool is_scalar(const JsonValue& value) noexcept
        {
            return value.kind() != JsonKind::Array && value.kind() != JsonKind::Object;
        }

        /// @brief Appends whole number @p value to @p out.
        template <class Whole>
        void append_whole(Whole value, std::string& out)
        {
            std::array<char, 24> buffer{};
            const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
            out.append(buffer.data(), result.ptr);
        }
    } // namespace

    JsonWriter::JsonWriter(bool pretty) : pretty_(pretty)
    {

    }

    void JsonWriter::new_line()
    {
        text_.push_back('\n');
        text_.append(levels_.size() * 2U, ' ');
    }

    void JsonWriter::before_item()
    {
        if (after_key_)
        {
            after_key_ = false;
            return;
        }
        if (levels_.empty())
            return;
        Level& level = levels_.back();
        if (!level.empty)
            text_.push_back(',');
        if (pretty_ && level.inline_layout && !level.empty)
            text_.push_back(' ');
        if (pretty_ && !level.inline_layout)
            new_line();
        level.empty = false;
    }

    void JsonWriter::open(char open, JsonLayout layout)
    {
        before_item();
        text_.push_back(open);
        const bool parent_inline = !levels_.empty() && levels_.back().inline_layout;
        levels_.push_back(Level{layout == JsonLayout::Inline || parent_inline, true});
    }

    void JsonWriter::close(char close)
    {
        const Level level = levels_.back();
        levels_.pop_back();
        if (pretty_ && !level.inline_layout && !level.empty)
            new_line();
        text_.push_back(close);
    }

    void JsonWriter::begin_object(JsonLayout layout)
    {
        open('{', layout);
    }

    void JsonWriter::end_object()
    {
        close('}');
    }

    void JsonWriter::begin_array(JsonLayout layout)
    {
        open('[', layout);
    }

    void JsonWriter::end_array()
    {
        close(']');
    }

    void JsonWriter::key(std::string_view name)
    {
        before_item();
        append_quoted(name);
        text_.push_back(':');
        if (pretty_)
            text_.push_back(' ');
        after_key_ = true;
    }

    void JsonWriter::null()
    {
        before_item();
        text_.append("null");
    }

    void JsonWriter::boolean(bool value)
    {
        before_item();
        text_.append(value ? "true" : "false");
    }

    void JsonWriter::integer(std::int64_t value)
    {
        before_item();
        append_whole(value, text_);
    }

    void JsonWriter::unsigned_integer(std::uint64_t value)
    {
        before_item();
        append_whole(value, text_);
    }

    void JsonWriter::real(double value)
    {
        if (!std::isfinite(value))
        {
            null();
            return;
        }
        before_item();
        std::array<char, 32> buffer{};
        const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
        const auto length = static_cast<std::size_t>(result.ptr - buffer.data());
        const std::string_view written(buffer.data(), length);
        text_.append(written);
        // A real written without '.' or an exponent would read back as a whole number.
        if (written.find_first_of(".e") == std::string_view::npos)
            text_.append(".0");
    }

    void JsonWriter::string(std::string_view value)
    {
        before_item();
        append_quoted(value);
    }

    void JsonWriter::append_quoted(std::string_view value)
    {
        static constexpr char K_HEX[] = "0123456789abcdef";
        text_.push_back('"');
        for (const char character : value)
        {
            const auto code = static_cast<unsigned char>(character);
            switch (character)
            {
                case '"': text_.append("\\\""); break;
                case '\\': text_.append("\\\\"); break;
                case '\n': text_.append("\\n"); break;
                case '\r': text_.append("\\r"); break;
                case '\t': text_.append("\\t"); break;
                case '\b': text_.append("\\b"); break;
                case '\f': text_.append("\\f"); break;
                default:
                    if (code < 0x20U)
                    {
                        text_.append("\\u00");
                        text_.push_back(K_HEX[code >> 4U]);
                        text_.push_back(K_HEX[code & 0x0FU]);
                    }
                    else
                    {
                        text_.push_back(character);
                    }
            }
        }
        text_.push_back('"');
    }

    void JsonWriter::value(const JsonValue& value)
    {
        switch (value.kind())
        {
            case JsonKind::Null: null(); return;
            case JsonKind::Bool: boolean(*value.as_bool()); return;
            case JsonKind::Integer: integer(*value.as_integer()); return;
            case JsonKind::Unsigned: unsigned_integer(*value.as_unsigned()); return;
            case JsonKind::Real: real(*value.as_real()); return;
            case JsonKind::String: string(*value.as_string()); return;
            case JsonKind::Array:
            {
                const std::vector<JsonValue>& elements = *value.as_array();
                bool scalars = true;
                for (const JsonValue& element : elements)
                    scalars = scalars && is_scalar(element);
                begin_array(scalars ? JsonLayout::Inline : JsonLayout::Block);
                for (const JsonValue& element : elements)
                    this->value(element);
                end_array();
                return;
            }
            case JsonKind::Object:
                begin_object();
                for (const JsonMember& member : *value.as_object())
                {
                    key(member.key);
                    this->value(member.value);
                }
                end_object();
                return;
        }
    }

    const std::string& JsonWriter::text() const noexcept
    {
        return text_;
    }

    std::string JsonWriter::take()
    {
        std::string text = std::move(text_);
        text_.clear();
        levels_.clear();
        after_key_ = false;
        return text;
    }
} // namespace SushiProfiler
