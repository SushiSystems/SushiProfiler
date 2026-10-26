/**************************************************************************/
/* registry.cpp                                                           */
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
 * @file registry.cpp
 * @brief Looks factories up by name and makes the built-in writers from their settings.
 * @author Mustafa Garip
 * @see modules/session/registry/README.md
 */

#include <SushiProfiler/registry/registry.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <utility>

#include <SushiProfiler/json/json_field_reader.hpp>
#include <SushiProfiler/record_writers/record_file_writer.hpp>
#include <SushiProfiler/record_writers/terminal_writer.hpp>
#include <SushiProfiler/stream_writers/chrome_trace_writer.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns the keys of @p map, in its order.
        template <class Map>
        std::vector<std::string> keys_of(const Map& map)
        {
            std::vector<std::string> keys;
            for (const auto& entry : map)
                keys.push_back(entry.first);
            return keys;
        }

        /// @brief Reads `path` from @p settings, joined to @p output when relative.
        std::string output_path(const JsonValue& settings, const std::string& output,
                                std::string_view writer, std::string& error)
        {
            JsonFieldReader fields("writers." + std::string(writer));
            std::string path;
            if (!fields.object(settings, {"path"}) || !fields.text(settings, "path", path))
            {
                error = fields.error();
                return std::string();
            }
            const std::filesystem::path relative(path);
            if (output.empty() || relative.is_absolute())
                return path;
            return (std::filesystem::path(output) / relative).generic_string();
        }
    } // namespace

    bool Registry::add_source(std::string name, SourceFactory factory)
    {
        return sources_.emplace(std::move(name), std::move(factory)).second;
    }

    bool Registry::add_stream_writer(std::string name, StreamWriterFactory factory)
    {
        if (record_writers_.count(name) != 0U)
            return false;
        return stream_writers_.emplace(std::move(name), std::move(factory)).second;
    }

    bool Registry::add_record_writer(std::string name, RecordWriterFactory factory)
    {
        if (stream_writers_.count(name) != 0U)
            return false;
        return record_writers_.emplace(std::move(name), std::move(factory)).second;
    }

    void Registry::add_contributor(const EnvironmentContributor& contributor)
    {
        contributors_.push_back(&contributor);
    }

    Made<Source> Registry::make_source(std::string_view name, const JsonValue& settings) const
    {
        const auto found = sources_.find(name);
        if (found == sources_.end())
            return Made<Source>{nullptr, unknown("source", name, keys_of(sources_))};
        return found->second(settings);
    }

    bool Registry::streams(std::string_view name) const
    {
        return stream_writers_.find(name) != stream_writers_.end();
    }

    Made<StreamWriter> Registry::make_stream_writer(std::string_view name,
                                                    const JsonValue& settings,
                                                    const std::string& output) const
    {
        const auto found = stream_writers_.find(name);
        if (found == stream_writers_.end())
            return Made<StreamWriter>{nullptr, unknown("writer", name, writer_names())};
        return found->second(settings, output);
    }

    Made<RecordWriter> Registry::make_record_writer(std::string_view name,
                                                    const JsonValue& settings,
                                                    const std::string& output) const
    {
        const auto found = record_writers_.find(name);
        if (found == record_writers_.end())
            return Made<RecordWriter>{nullptr, unknown("writer", name, writer_names())};
        return found->second(settings, output);
    }

    const std::vector<const EnvironmentContributor*>& Registry::contributors() const noexcept
    {
        return contributors_;
    }

    std::string Registry::unknown(std::string_view kind, std::string_view name,
                                  const std::vector<std::string>& known)
    {
        std::string text =
            "unknown " + std::string(kind) + " \"" + std::string(name) + "\"; known:";
        if (known.empty())
            return text + " none";
        for (std::size_t index = 0U; index < known.size(); ++index)
            text += (index == 0U ? " " : ", ") + known[index];
        return text;
    }

    std::vector<std::string> Registry::writer_names() const
    {
        std::vector<std::string> names = keys_of(stream_writers_);
        const std::vector<std::string> records = keys_of(record_writers_);
        names.insert(names.end(), records.begin(), records.end());
        std::sort(names.begin(), names.end());
        return names;
    }

    void register_builtin_writers(Registry& registry, std::ostream& terminal)
    {
        const auto make_terminal = [&terminal](const JsonValue& settings, const std::string&)
        {
            JsonFieldReader fields("writers.terminal");
            std::uint32_t top = 20U;
            if (!fields.object_within(settings, {"top"}) ||
                (JsonFieldReader::has(settings, "top") && !fields.whole(settings, "top", top)))
                return Made<RecordWriter>{nullptr, fields.error()};
            return Made<RecordWriter>{std::make_unique<TerminalWriter>(terminal, top), ""};
        };
        const auto make_record = [](const JsonValue& settings, const std::string& output)
        {
            std::string error;
            const std::string path = output_path(settings, output, "record", error);
            if (!error.empty())
                return Made<RecordWriter>{nullptr, error};
            return Made<RecordWriter>{std::make_unique<RecordFileWriter>(path), ""};
        };
        const auto make_chrome_trace = [](const JsonValue& settings, const std::string& output)
        {
            std::string error;
            const std::string path = output_path(settings, output, "chrome_trace", error);
            if (!error.empty())
                return Made<StreamWriter>{nullptr, error};
            return Made<StreamWriter>{std::make_unique<ChromeTraceWriter>(path), ""};
        };
        static_cast<void>(registry.add_record_writer("terminal", make_terminal));
        static_cast<void>(registry.add_record_writer("record", make_record));
        static_cast<void>(registry.add_stream_writer("chrome_trace", make_chrome_trace));
    }
} // namespace SushiProfiler
