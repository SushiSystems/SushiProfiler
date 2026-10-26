/**************************************************************************/
/* json_field_reader.hpp                                                  */
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
 * @file json_field_reader.hpp
 * @brief Declares the strict field reader that names the JSON path of each refusal.
 * @author Mustafa Garip
 * @see modules/foundation/json/README.md
 */

#pragma once

#include <SushiProfiler/json/json_value.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Reads JSON fields by kind and range, keeping the first refusal with its path.
     *
     * Every read names where it is: a keyed read adds `.key` and an element read adds `[i]` for its
     * duration, so a refusal reads `periods[2].spans.track: <reason>`.
     */
    class SUSHIPROFILER_API JsonFieldReader final
    {
        public:
            /// @brief Builds a reader that names the document @p root when the path is empty.
            explicit JsonFieldReader(std::string root = "document");

            /// @brief Returns the first refusal, or an empty string.
            [[nodiscard]] const std::string& error() const noexcept;

            /// @brief Records @p reason at the current path; returns false.
            bool fail(std::string_view reason);

            /// @brief Refuses @p value unless it is an object holding exactly @p keys.
            [[nodiscard]] bool object(const JsonValue& value,
                                      std::initializer_list<std::string_view> keys);

            /// @brief Refuses @p value unless it is an object whose keys are all among @p keys.
            [[nodiscard]] bool object_within(const JsonValue& value,
                                             std::initializer_list<std::string_view> keys);

            /// @brief Returns whether @p object, which object_within() has checked, has @p key.
            [[nodiscard]] static bool has(const JsonValue& object, std::string_view key) noexcept;

            /// @brief Reads member @p key of @p object as a string.
            [[nodiscard]] bool text(const JsonValue& object, std::string_view key,
                                    std::string& out);

            /// @brief Reads member @p key of @p object as a bool.
            [[nodiscard]] bool flag(const JsonValue& object, std::string_view key, bool& out);

            /// @brief Reads member @p key of @p object as a real or a non-finite word.
            [[nodiscard]] bool real(const JsonValue& object, std::string_view key, double& out);

            /// @brief Reads member @p key of @p object as an unsigned whole number of type Whole.
            template <class Whole>
            [[nodiscard]] bool whole(const JsonValue& object, std::string_view key, Whole& out)
            {
                const Segment segment(*this, key);
                std::uint64_t value = 0U;
                if (!whole_value(member(object, key), std::numeric_limits<Whole>::max(), value))
                    return false;
                out = static_cast<Whole>(value);
                return true;
            }

            /// @brief Reads member @p key of @p object as an array of unsigned whole numbers.
            template <class Whole>
            [[nodiscard]] bool wholes(const JsonValue& object, std::string_view key,
                                      std::vector<Whole>& out)
            {
                const Segment segment(*this, key);
                const std::vector<JsonValue>* const elements = array(member(object, key));
                if (elements == nullptr)
                    return false;
                out.reserve(elements->size());
                for (std::size_t index = 0U; index < elements->size(); ++index)
                {
                    const Segment at(*this, index);
                    std::uint64_t value = 0U;
                    if (!whole_value((*elements)[index], std::numeric_limits<Whole>::max(), value))
                        return false;
                    out.push_back(static_cast<Whole>(value));
                }
                return true;
            }

            /// @brief Reads member @p key of @p object as whole numbers in [minimum, maximum].
            [[nodiscard]] bool integers(const JsonValue& object, std::string_view key,
                                        std::int64_t minimum, std::int64_t maximum,
                                        std::vector<std::int64_t>& out);

            /// @brief Reads member @p key of @p object as an array of reals.
            [[nodiscard]] bool reals(const JsonValue& object, std::string_view key,
                                     std::vector<double>& out);

            /// @brief Reads member @p key of @p object as an array of strings.
            [[nodiscard]] bool texts(const JsonValue& object, std::string_view key,
                                     std::vector<std::string>& out);

            /// @brief Calls @p visit on each element of array member @p key, stopping at a refusal.
            template <class Visit>
            [[nodiscard]] bool each(const JsonValue& object, std::string_view key, Visit visit)
            {
                const Segment segment(*this, key);
                const std::vector<JsonValue>* const elements = array(member(object, key));
                if (elements == nullptr)
                    return false;
                for (std::size_t index = 0U; index < elements->size(); ++index)
                {
                    const Segment at(*this, index);
                    if (!visit((*elements)[index]))
                        return false;
                }
                return true;
            }

            /// @brief Refuses column @p key unless it has @p expected entries.
            [[nodiscard]] bool length(std::string_view key, std::size_t actual,
                                      std::size_t expected);

            /// @brief Refuses id @p id in member @p key unless it is below @p count.
            [[nodiscard]] bool below(std::string_view key, std::uint64_t id, std::size_t count,
                                     std::string_view table);

            /// @brief Refuses column @p key unless each id in it is below @p count.
            template <class ID>
            [[nodiscard]] bool all_below(std::string_view key, const std::vector<ID>& ids,
                                         std::size_t count, std::string_view table)
            {
                const Segment segment(*this, key);
                for (std::size_t index = 0U; index < ids.size(); ++index)
                {
                    const Segment at(*this, index);
                    if (static_cast<std::uint64_t>(ids[index]) >= count)
                        return fail("points past the " + std::string(table) + " table");
                }
                return true;
            }

            /// @brief Adds one path segment for the life of the object.
            class SUSHIPROFILER_API Segment final
            {
                public:
                    /// @brief Adds `.key`.
                    Segment(JsonFieldReader& decoder, std::string_view key);

                    /// @brief Adds `[index]`.
                    Segment(JsonFieldReader& decoder, std::size_t index);

                    ~Segment();

                    Segment(const Segment&) = delete;
                    Segment& operator=(const Segment&) = delete;
                    Segment(Segment&&) = delete;
                    Segment& operator=(Segment&&) = delete;

                private:
                    /// @brief The decoder whose path grows.
                    JsonFieldReader& decoder_;

                    /// @brief The path's length before the segment.
                    std::size_t restore_;
            };

            /// @brief Returns member @p key of @p object, which object() has checked.
            [[nodiscard]] static const JsonValue& member(const JsonValue& object,
                                                         std::string_view key) noexcept;

        private:
            /// @brief Returns @p value's elements, or refuses it when it is not an array.
            const std::vector<JsonValue>* array(const JsonValue& value);

            /// @brief Reads @p value as an unsigned whole number no larger than @p maximum.
            bool whole_value(const JsonValue& value, std::uint64_t maximum, std::uint64_t& out);

            /// @brief Reads @p value as a real, or as the format's non-finite word.
            bool real_value(const JsonValue& value, double& out);

            /// @brief Refuses the first key of @p members not among @p keys.
            bool known_keys(const std::vector<JsonMember>& members,
                            std::initializer_list<std::string_view> keys);

            /// @brief What an empty path is called in a refusal.
            std::string root_;

            /// @brief The current path.
            std::string path_;

            /// @brief The first refusal.
            std::string error_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
