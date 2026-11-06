/**************************************************************************/
/* json_value.cpp                                                         */
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
 * @file json_value.cpp
 * @brief Builds JSON values, reads them by kind and finds object members.
 * @author Mustafa Garip
 * @see modules/foundation/json/README.md
 */

#include <SushiProfiler/json/json_value.hpp>

#include <utility>

namespace SushiProfiler
{
    JsonValue::JsonValue(Content content) : content_(std::move(content))
    {

    }

    JsonValue JsonValue::boolean(bool value)
    {
        return JsonValue(Content(std::in_place_index<1>, value));
    }

    JsonValue JsonValue::integer(std::int64_t value)
    {
        return JsonValue(Content(std::in_place_index<2>, value));
    }

    JsonValue JsonValue::unsigned_integer(std::uint64_t value)
    {
        return JsonValue(Content(std::in_place_index<3>, value));
    }

    JsonValue JsonValue::real(double value)
    {
        return JsonValue(Content(std::in_place_index<4>, value));
    }

    JsonValue JsonValue::string(std::string value)
    {
        return JsonValue(Content(std::in_place_index<5>, std::move(value)));
    }

    JsonValue JsonValue::array()
    {
        return JsonValue(Content(std::in_place_index<6>));
    }

    JsonValue JsonValue::object()
    {
        return JsonValue(Content(std::in_place_index<7>));
    }

    JsonKind JsonValue::kind() const noexcept
    {
        return static_cast<JsonKind>(content_.index());
    }

    const bool* JsonValue::as_bool() const noexcept
    {
        return std::get_if<1>(&content_);
    }

    const std::int64_t* JsonValue::as_integer() const noexcept
    {
        return std::get_if<2>(&content_);
    }

    const std::uint64_t* JsonValue::as_unsigned() const noexcept
    {
        return std::get_if<3>(&content_);
    }

    const double* JsonValue::as_real() const noexcept
    {
        return std::get_if<4>(&content_);
    }

    const std::string* JsonValue::as_string() const noexcept
    {
        return std::get_if<5>(&content_);
    }

    const std::vector<JsonValue>* JsonValue::as_array() const noexcept
    {
        return std::get_if<6>(&content_);
    }

    const std::vector<JsonMember>* JsonValue::as_object() const noexcept
    {
        return std::get_if<7>(&content_);
    }

    bool JsonValue::to_uint64(std::uint64_t& out) const noexcept
    {
        if (const std::uint64_t* const value = as_unsigned())
        {
            out = *value;
            return true;
        }
        const std::int64_t* const value = as_integer();
        if (value == nullptr || *value < 0)
            return false;
        out = static_cast<std::uint64_t>(*value);
        return true;
    }

    bool JsonValue::to_double(double& out) const noexcept
    {
        if (const double* const value = as_real())
            out = *value;
        else if (const std::int64_t* const whole = as_integer())
            out = static_cast<double>(*whole);
        else if (const std::uint64_t* const large = as_unsigned())
            out = static_cast<double>(*large);
        else
            return false;
        return true;
    }

    const JsonValue* JsonValue::find(std::string_view key) const noexcept
    {
        const std::vector<JsonMember>* const members = as_object();
        if (members == nullptr)
            return nullptr;
        for (const JsonMember& member : *members)
        {
            if (member.key == key)
                return &member.value;
        }
        return nullptr;
    }

    void JsonValue::push_back(JsonValue element)
    {
        std::get<6>(content_).push_back(std::move(element));
    }

    void JsonValue::set(std::string key, JsonValue member)
    {
        std::vector<JsonMember>& members = std::get<7>(content_);
        for (JsonMember& existing : members)
        {
            if (existing.key == key)
            {
                existing.value = std::move(member);
                return;
            }
        }
        members.push_back(JsonMember{std::move(key), std::move(member)});
    }

    bool JsonValue::operator==(const JsonValue& other) const
    {
        return content_ == other.content_;
    }

    bool JsonValue::operator!=(const JsonValue& other) const
    {
        return !(*this == other);
    }

    bool JsonMember::operator==(const JsonMember& other) const
    {
        return key == other.key && value == other.value;
    }
} // namespace SushiProfiler
