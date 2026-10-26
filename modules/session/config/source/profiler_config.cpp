/**************************************************************************/
/* profiler_config.cpp                                                    */
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
 * @file profiler_config.cpp
 * @brief Reads the configuration's optional keys through the strict field reader.
 * @author Mustafa Garip
 * @see docs/reference/CONFIG_FORMAT.md
 */

#include <SushiProfiler/config/profiler_config.hpp>

#include <utility>

#include <SushiProfiler/json/json_field_reader.hpp>
#include <SushiProfiler/json/json_reader.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Reads one configuration, keeping the default of every key it lacks.
        class ConfigParser final
        {
            public:
                /// @brief Reads @p root into @p out.
                bool parse(const JsonValue& root, ProfilerConfig& out)
                {
                    if (!fields_.object_within(root, {"sushiprofiler", "enabled", "level",
                                                      "groups", "timeline", "window", "sources",
                                                      "analyses", "writers", "output", "buffers"}))
                        return false;
                    if (!JsonFieldReader::has(root, "sushiprofiler"))
                        return fields_.fail("missing key \"sushiprofiler\"");
                    std::uint32_t version = 0U;
                    if (!fields_.whole(root, "sushiprofiler", version))
                        return false;
                    if (version != K_CONFIG_FORMAT_VERSION)
                    {
                        const JsonFieldReader::Segment segment(fields_, "sushiprofiler");
                        return fields_.fail("version " + std::to_string(version) +
                                            " is not the version this build reads, " +
                                            std::to_string(K_CONFIG_FORMAT_VERSION));
                    }
                    return optional_flag(root, "enabled", out.enabled) &&
                           ranged(root, "level", 0U, 3U, out.level) &&
                           optional_texts(root, "groups", out.groups) &&
                           optional_text(root, "timeline", out.timeline) &&
                           window(root, out.window) &&
                           named(root, "sources", out.sources) &&
                           optional_texts(root, "analyses", out.analyses) &&
                           named(root, "writers", out.writers) &&
                           optional_text(root, "output", out.output) &&
                           buffers(root, out.buffers);
                }

                /// @brief Returns the first refusal.
                [[nodiscard]] const std::string& error() const noexcept
                {
                    return fields_.error();
                }

            private:
                /// @brief Reads bool @p key of @p object when it is there.
                bool optional_flag(const JsonValue& object, std::string_view key, bool& out)
                {
                    return !JsonFieldReader::has(object, key) || fields_.flag(object, key, out);
                }

                /// @brief Reads string @p key of @p object when it is there.
                bool optional_text(const JsonValue& object, std::string_view key, std::string& out)
                {
                    return !JsonFieldReader::has(object, key) || fields_.text(object, key, out);
                }

                /// @brief Reads string array @p key of @p object when it is there.
                bool optional_texts(const JsonValue& object, std::string_view key,
                                    std::vector<std::string>& out)
                {
                    if (!JsonFieldReader::has(object, key))
                        return true;
                    out.clear();
                    return fields_.texts(object, key, out);
                }

                /// @brief Reads whole @p key of @p object in [minimum, maximum] when it is there.
                template <class Whole>
                bool ranged(const JsonValue& object, std::string_view key, std::uint64_t minimum,
                            std::uint64_t maximum, Whole& out)
                {
                    if (!JsonFieldReader::has(object, key))
                        return true;
                    std::uint64_t value = 0U;
                    if (!fields_.whole(object, key, value))
                        return false;
                    if (value < minimum || value > maximum)
                    {
                        const JsonFieldReader::Segment segment(fields_, key);
                        return fields_.fail("out of range " + std::to_string(minimum) + ".." +
                                            std::to_string(maximum));
                    }
                    out = static_cast<Whole>(value);
                    return true;
                }

                /// @brief Reads the window schedule when it is there.
                bool window(const JsonValue& root, WindowSchedule& out)
                {
                    if (!JsonFieldReader::has(root, "window"))
                        return true;
                    const JsonFieldReader::Segment segment(fields_, "window");
                    const JsonValue& value = JsonFieldReader::member(root, "window");
                    constexpr std::uint64_t K_MOST = 0xFFFFFFFFU;
                    return fields_.object_within(value,
                                                 {"skip", "wait", "warmup", "active", "repeat"}) &&
                           ranged(value, "skip", 0U, K_MOST, out.skip) &&
                           ranged(value, "wait", 0U, K_MOST, out.wait) &&
                           ranged(value, "warmup", 0U, K_MOST, out.warmup) &&
                           ranged(value, "active", 1U, K_MOST, out.active) &&
                           ranged(value, "repeat", 0U, K_MOST, out.repeat);
                }

                /// @brief Reads the buffer sizes when they are there.
                bool buffers(const JsonValue& root, BufferSettings& out)
                {
                    if (!JsonFieldReader::has(root, "buffers"))
                        return true;
                    const JsonFieldReader::Segment segment(fields_, "buffers");
                    const JsonValue& value = JsonFieldReader::member(root, "buffers");
                    return fields_.object_within(value,
                                                 {"ring_events", "attribute_bytes", "linger_ms"}) &&
                           ranged(value, "ring_events", 64U, 1U << 24U, out.ring_events) &&
                           ranged(value, "attribute_bytes", 0U, 1ULL << 32U, out.attribute_bytes) &&
                           ranged(value, "linger_ms", 0U, 60'000U, out.linger_ms);
                }

                /// @brief Reads object @p key of name to settings object when it is there.
                bool named(const JsonValue& root, std::string_view key,
                           std::vector<NamedSettings>& out)
                {
                    if (!JsonFieldReader::has(root, key))
                        return true;
                    const JsonFieldReader::Segment segment(fields_, key);
                    const std::vector<JsonMember>* const members =
                        JsonFieldReader::member(root, key).as_object();
                    if (members == nullptr)
                        return fields_.fail("an object of names to settings was expected");
                    for (const JsonMember& member : *members)
                    {
                        if (member.value.kind() != JsonKind::Object)
                        {
                            const JsonFieldReader::Segment entry(fields_, member.key);
                            return fields_.fail("settings must be an object");
                        }
                        out.push_back(NamedSettings{member.key, member.value});
                    }
                    return true;
                }

                /// @brief The field reader.
                JsonFieldReader fields_{"config"};
        };
    } // namespace

    ConfigReadResult read_profiler_config(std::string_view text)
    {
        ConfigReadResult result;
        const JsonReadResult json = read_json(text);
        if (!json.ok())
        {
            result.error = "line " + std::to_string(json.error.line) + ", column " +
                           std::to_string(json.error.column) + ": " + json.error.message;
            return result;
        }
        ProfilerConfig config;
        ConfigParser parser;
        if (parser.parse(*json.value, config))
            result.config = std::move(config);
        else
            result.error = parser.error();
        return result;
    }
} // namespace SushiProfiler
