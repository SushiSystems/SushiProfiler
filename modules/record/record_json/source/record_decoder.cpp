/**************************************************************************/
/* record_decoder.cpp                                                     */
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
 * @file record_decoder.cpp
 * @brief Reads record fields by kind and range and words each refusal with its JSON path.
 * @author Mustafa Garip
 * @see docs/reference/RECORD_FORMAT.md
 */

#include "record_decoder.hpp"

#include <cmath>

namespace SushiProfiler::Detail
{
    namespace
    {
        /// @brief Returns the kind's name as a refusal says it.
        std::string_view kind_name(JsonKind kind) noexcept
        {
            switch (kind)
            {
                case JsonKind::Null: return "null";
                case JsonKind::Bool: return "a bool";
                case JsonKind::Integer: return "a whole number";
                case JsonKind::Unsigned: return "a whole number";
                case JsonKind::Real: return "a real";
                case JsonKind::String: return "a string";
                case JsonKind::Array: return "an array";
                case JsonKind::Object: return "an object";
            }
            return "a value";
        }
    } // namespace

    RecordDecoder::Segment::Segment(RecordDecoder& decoder, std::string_view key)
        : decoder_(decoder), restore_(decoder.path_.size())
    {
        if (!decoder_.path_.empty())
            decoder_.path_.push_back('.');
        decoder_.path_.append(key);
    }

    RecordDecoder::Segment::Segment(RecordDecoder& decoder, std::size_t index)
        : decoder_(decoder), restore_(decoder.path_.size())
    {
        decoder_.path_.push_back('[');
        decoder_.path_.append(std::to_string(index));
        decoder_.path_.push_back(']');
    }

    RecordDecoder::Segment::~Segment()
    {
        decoder_.path_.resize(restore_);
    }

    const std::string& RecordDecoder::error() const noexcept
    {
        return error_;
    }

    bool RecordDecoder::fail(std::string_view reason)
    {
        if (error_.empty())
        {
            error_ = path_.empty() ? std::string("record") : path_;
            error_.append(": ");
            error_.append(reason);
        }
        return false;
    }

    const JsonValue& RecordDecoder::member(const JsonValue& object, std::string_view key) noexcept
    {
        return *object.find(key);
    }

    bool RecordDecoder::object(const JsonValue& value, std::initializer_list<std::string_view> keys)
    {
        const std::vector<JsonMember>* const members = value.as_object();
        if (members == nullptr)
            return fail(std::string("an object was expected, found ") +
                        std::string(kind_name(value.kind())));
        for (const JsonMember& found : *members)
        {
            bool known = false;
            for (const std::string_view key : keys)
                known = known || found.key == key;
            if (!known)
                return fail("unknown key \"" + found.key + "\"");
        }
        for (const std::string_view key : keys)
        {
            if (value.find(key) == nullptr)
                return fail("missing key \"" + std::string(key) + "\"");
        }
        return true;
    }

    const std::vector<JsonValue>* RecordDecoder::array(const JsonValue& value)
    {
        const std::vector<JsonValue>* const elements = value.as_array();
        if (elements == nullptr)
            fail(std::string("an array was expected, found ") +
                 std::string(kind_name(value.kind())));
        return elements;
    }

    bool RecordDecoder::whole_value(const JsonValue& value, std::uint64_t maximum,
                                    std::uint64_t& out)
    {
        if (value.kind() != JsonKind::Integer && value.kind() != JsonKind::Unsigned)
            return fail(std::string("a whole number was expected, found ") +
                        std::string(kind_name(value.kind())));
        if (!value.to_uint64(out) || out > maximum)
            return fail("out of range 0.." + std::to_string(maximum));
        return true;
    }

    bool RecordDecoder::real_value(const JsonValue& value, double& out)
    {
        if (const std::string* const word = value.as_string())
        {
            if (*word == "NaN")
                out = std::nan("");
            else if (*word == "Infinity")
                out = HUGE_VAL;
            else if (*word == "-Infinity")
                out = -HUGE_VAL;
            else
                return fail("\"" + *word + "\" is not NaN, Infinity or -Infinity");
            return true;
        }
        if (!value.to_double(out))
            return fail(std::string("a real was expected, found ") +
                        std::string(kind_name(value.kind())));
        return true;
    }

    bool RecordDecoder::text(const JsonValue& object, std::string_view key, std::string& out)
    {
        const Segment segment(*this, key);
        const std::string* const value = member(object, key).as_string();
        if (value == nullptr)
            return fail(std::string("a string was expected, found ") +
                        std::string(kind_name(member(object, key).kind())));
        out = *value;
        return true;
    }

    bool RecordDecoder::flag(const JsonValue& object, std::string_view key, bool& out)
    {
        const Segment segment(*this, key);
        const bool* const value = member(object, key).as_bool();
        if (value == nullptr)
            return fail(std::string("a bool was expected, found ") +
                        std::string(kind_name(member(object, key).kind())));
        out = *value;
        return true;
    }

    bool RecordDecoder::real(const JsonValue& object, std::string_view key, double& out)
    {
        const Segment segment(*this, key);
        return real_value(member(object, key), out);
    }

    bool RecordDecoder::integers(const JsonValue& object, std::string_view key,
                                 std::int64_t minimum, std::int64_t maximum,
                                 std::vector<std::int64_t>& out)
    {
        const Segment segment(*this, key);
        const std::vector<JsonValue>* const elements = array(member(object, key));
        if (elements == nullptr)
            return false;
        out.reserve(elements->size());
        for (std::size_t index = 0U; index < elements->size(); ++index)
        {
            const Segment at(*this, index);
            const std::int64_t* const value = (*elements)[index].as_integer();
            if (value == nullptr || *value < minimum || *value > maximum)
            {
                return fail("a whole number in " + std::to_string(minimum) + ".." +
                            std::to_string(maximum) + " was expected");
            }
            out.push_back(*value);
        }
        return true;
    }

    bool RecordDecoder::reals(const JsonValue& object, std::string_view key,
                              std::vector<double>& out)
    {
        const Segment segment(*this, key);
        const std::vector<JsonValue>* const elements = array(member(object, key));
        if (elements == nullptr)
            return false;
        out.reserve(elements->size());
        for (std::size_t index = 0U; index < elements->size(); ++index)
        {
            const Segment at(*this, index);
            double value = 0.0;
            if (!real_value((*elements)[index], value))
                return false;
            out.push_back(value);
        }
        return true;
    }

    bool RecordDecoder::texts(const JsonValue& object, std::string_view key,
                              std::vector<std::string>& out)
    {
        const Segment segment(*this, key);
        const std::vector<JsonValue>* const elements = array(member(object, key));
        if (elements == nullptr)
            return false;
        out.reserve(elements->size());
        for (std::size_t index = 0U; index < elements->size(); ++index)
        {
            const Segment at(*this, index);
            const std::string* const value = (*elements)[index].as_string();
            if (value == nullptr)
                return fail("a string was expected");
            out.push_back(*value);
        }
        return true;
    }

    bool RecordDecoder::length(std::string_view key, std::size_t actual, std::size_t expected)
    {
        if (actual == expected)
            return true;
        const Segment segment(*this, key);
        return fail("has " + std::to_string(actual) + " entries where " +
                    std::to_string(expected) + " were expected");
    }

    bool RecordDecoder::below(std::string_view key, std::uint64_t id, std::size_t count,
                              std::string_view table)
    {
        if (id < count)
            return true;
        const Segment segment(*this, key);
        return fail(std::to_string(id) + " points past the " + std::string(table) + " table");
    }
} // namespace SushiProfiler::Detail
