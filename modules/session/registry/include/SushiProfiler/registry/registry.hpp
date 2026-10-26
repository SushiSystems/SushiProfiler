/**************************************************************************/
/* registry.hpp                                                           */
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
 * @file registry.hpp
 * @brief Declares the registry of the sources, writers and contributors a session can be given.
 * @author Mustafa Garip
 * @see modules/session/registry/README.md
 */

#pragma once

#include <SushiProfiler/environment/environment_contributor.hpp>
#include <SushiProfiler/json/json_value.hpp>
#include <SushiProfiler/record_writers/record_writer.hpp>
#include <SushiProfiler/sources/source.hpp>
#include <SushiProfiler/stream_writers/stream_writer.hpp>
#include <SushiProfiler/version/export.hpp>

#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief A part a factory made, or why it could not.
    template <class Part>
    struct Made
    {
        std::unique_ptr<Part> part; /**< The part, or null. */
        std::string error;          /**< Why it was not made, or empty. */
    };

    /// @brief Makes a source from its settings object.
    using SourceFactory = std::function<Made<Source>(const JsonValue& settings)>;

    /// @brief Makes a stream writer from its settings and the output folder.
    using StreamWriterFactory =
        std::function<Made<StreamWriter>(const JsonValue& settings, const std::string& output)>;

    /// @brief Makes a record writer from its settings and the output folder.
    using RecordWriterFactory =
        std::function<Made<RecordWriter>(const JsonValue& settings, const std::string& output)>;

    /// @brief Holds the factories a configuration names and the contributors every session runs.
    class SUSHIPROFILER_API Registry final
    {
        public:
            /// @brief Adds @p factory as source @p name; returns false when the name is taken.
            bool add_source(std::string name, SourceFactory factory);

            /// @brief Adds @p factory as writer @p name; returns false when the name is taken.
            bool add_stream_writer(std::string name, StreamWriterFactory factory);

            /// @brief Adds @p factory as writer @p name; returns false when the name is taken.
            bool add_record_writer(std::string name, RecordWriterFactory factory);

            /// @brief Adds @p contributor, which must outlive every session started with it.
            void add_contributor(const EnvironmentContributor& contributor);

            /// @brief Makes source @p name, or says it is unknown and lists the known ones.
            [[nodiscard]] Made<Source> make_source(std::string_view name,
                                                   const JsonValue& settings) const;

            /// @brief Returns whether writer @p name streams, so the session runs it.
            [[nodiscard]] bool streams(std::string_view name) const;

            /// @brief Makes stream writer @p name, or says it is unknown and lists every writer.
            [[nodiscard]] Made<StreamWriter> make_stream_writer(std::string_view name,
                                                                const JsonValue& settings,
                                                                const std::string& output) const;

            /// @brief Makes record writer @p name, or says it is unknown and lists every writer.
            [[nodiscard]] Made<RecordWriter> make_record_writer(std::string_view name,
                                                                const JsonValue& settings,
                                                                const std::string& output) const;

            /// @brief Returns the contributors, in the order they were added.
            [[nodiscard]] const std::vector<const EnvironmentContributor*>& contributors()
                const noexcept;

        private:
            /// @brief Returns `unknown <kind> "<name>"; known: ...` over @p known.
            [[nodiscard]] static std::string unknown(std::string_view kind, std::string_view name,
                                                     const std::vector<std::string>& known);

            /// @brief Returns every writer name, sorted.
            [[nodiscard]] std::vector<std::string> writer_names() const;

            /// @brief The source factories by name.
            std::map<std::string, SourceFactory, std::less<>> sources_;

            /// @brief The stream writer factories by name.
            std::map<std::string, StreamWriterFactory, std::less<>> stream_writers_;

            /// @brief The record writer factories by name.
            std::map<std::string, RecordWriterFactory, std::less<>> record_writers_;

            /// @brief The contributors, in the order added.
            std::vector<const EnvironmentContributor*> contributors_;
    };

    /**
     * @brief Adds the writers this library holds: `terminal`, `record` and `chrome_trace`.
     *
     * `terminal` takes `top`, the rows per table, and writes to @p terminal, which must outlive
     * the registry. `record` and `chrome_trace` take `path`, joined to the output folder when it
     * is relative.
     */
    SUSHIPROFILER_API void register_builtin_writers(Registry& registry, std::ostream& terminal);
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
