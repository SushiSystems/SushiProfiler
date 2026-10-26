/**************************************************************************/
/* json_writer.hpp                                                        */
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
 * @file json_writer.hpp
 * @brief Declares the streaming JSON writer, compact or indented.
 * @author Mustafa Garip
 * @see modules/foundation/json/README.md
 */

#pragma once

#include <SushiProfiler/json/json_value.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace SushiProfiler
{
    /// @brief Says whether an indented writer breaks a container over lines.
    enum class JsonLayout : std::uint8_t
    {
        Block, /**< One element or member per line. */
        Inline /**< Every element on one line, as for a column of numbers. */
    };

    /**
     * @brief Writes one JSON document into a string, value by value.
     *
     * The caller keeps the grammar: a key before each member, containers closed in order. A real
     * that is not finite is written as null, since JSON has no NaN or infinity.
     */
    class JsonWriter final
    {
        public:
            /// @brief Builds a writer, indented by two spaces when @p pretty is set.
            explicit JsonWriter(bool pretty);

            /// @brief Opens an object.
            void begin_object(JsonLayout layout = JsonLayout::Block);

            /// @brief Closes the innermost object.
            void end_object();

            /// @brief Opens an array.
            void begin_array(JsonLayout layout = JsonLayout::Block);

            /// @brief Closes the innermost array.
            void end_array();

            /// @brief Writes the name of the next member.
            void key(std::string_view name);

            /// @brief Writes null.
            void null();

            /// @brief Writes true or false.
            void boolean(bool value);

            /// @brief Writes a signed whole number.
            void integer(std::int64_t value);

            /// @brief Writes an unsigned whole number.
            void unsigned_integer(std::uint64_t value);

            /// @brief Writes a real in its shortest exact form, always with '.' or an exponent.
            void real(double value);

            /// @brief Writes a string, escaping quotes, backslashes and control characters.
            void string(std::string_view value);

            /// @brief Writes @p value; an array of scalars stays on one line.
            void value(const JsonValue& value);

            /// @brief Returns the text written so far.
            [[nodiscard]] const std::string& text() const noexcept;

            /// @brief Moves the text out, leaving the writer empty.
            [[nodiscard]] std::string take();

        private:
            /// @brief One open container.
            struct Level
            {
                bool inline_layout = false; /**< Whether its elements share one line. */
                bool empty = true;          /**< Whether nothing was written in it yet. */
            };

            /// @brief Writes the separator and indentation before a value or a key.
            void before_item();

            /// @brief Appends @p value quoted and escaped, with no separator before it.
            void append_quoted(std::string_view value);

            /// @brief Opens a container with bracket @p open.
            void open(char open, JsonLayout layout);

            /// @brief Closes a container with bracket @p close.
            void close(char close);

            /// @brief Writes a new line and the indentation of the open containers.
            void new_line();

            /// @brief Whether containers are indented.
            bool pretty_;

            /// @brief Whether the last thing written was a key, so a value follows it directly.
            bool after_key_ = false;

            /// @brief The open containers, innermost last.
            std::vector<Level> levels_;

            /// @brief The text.
            std::string text_;
    };
} // namespace SushiProfiler
