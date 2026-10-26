/**************************************************************************/
/* json_reader.hpp                                                        */
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
 * @file json_reader.hpp
 * @brief Declares the JSON reader, which names the line and column of what it refuses.
 * @author Mustafa Garip
 * @see modules/foundation/json/README.md
 */

#pragma once

#include <SushiProfiler/json/json_value.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace SushiProfiler
{
    /// @brief The deepest nesting of arrays and objects the reader accepts.
    inline constexpr std::size_t K_JSON_MAX_DEPTH = 128U;

    /// @brief Where a document stops being JSON, and why.
    struct JsonError
    {
        std::size_t line = 0U;   /**< The line, from 1. */
        std::size_t column = 0U; /**< The column in bytes, from 1. */
        std::string message;     /**< What was expected or refused there. */
    };

    /// @brief The value a document holds, or the error that stopped the reader.
    struct JsonReadResult
    {
        std::optional<JsonValue> value; /**< The value, when the document is JSON. */
        JsonError error;                /**< The error, when it is not. */

        /// @brief Returns whether the document was read.
        [[nodiscard]] bool ok() const noexcept
        {
            return value.has_value();
        }
    };

    /**
     * @brief Reads one JSON document from @p text.
     *
     * A whole number without a fraction or exponent keeps 64 bits; a duplicate key, nesting past
     * K_JSON_MAX_DEPTH and anything after the value are refused.
     */
    [[nodiscard]] SUSHIPROFILER_API JsonReadResult read_json(std::string_view text);
} // namespace SushiProfiler
