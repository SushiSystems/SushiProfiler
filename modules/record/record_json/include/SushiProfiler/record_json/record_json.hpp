/**************************************************************************/
/* record_json.hpp                                                        */
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
 * @file record_json.hpp
 * @brief Declares the run record's JSON form: its writer and its strict reader.
 * @author Mustafa Garip
 * @see modules/record/record_json/README.md
 */

#pragma once

#include <SushiProfiler/record/run_record.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace SushiProfiler
{
    /// @brief The version of the record format this build writes and reads.
    inline constexpr std::uint32_t K_RECORD_FORMAT_VERSION = 1U;

    /// @brief A record read from text, or why it was refused.
    struct RecordReadResult
    {
        std::optional<RunRecord> record; /**< The record, when the text holds one. */
        std::string error;               /**< The JSON path of the refusal and its reason. */

        /// @brief Returns whether the record was read.
        [[nodiscard]] bool ok() const noexcept
        {
            return record.has_value();
        }
    };

    /**
     * @brief Returns @p record as indented JSON, columns on one line each.
     *
     * The same record always gives the same bytes. A real that is not finite is written as the
     * string `NaN`, `Infinity` or `-Infinity`; a NaN's payload is not kept.
     */
    [[nodiscard]] std::string write_record(const RunRecord& record);

    /**
     * @brief Reads a record, refusing what the format does not allow.
     *
     * Refused, each with its JSON path: a version other than K_RECORD_FORMAT_VERSION, an unknown
     * or missing key, a value of the wrong kind or out of range, columns of unequal length, and a
     * site, name, thread or parent id that points nowhere.
     */
    [[nodiscard]] RecordReadResult read_record(std::string_view text);
} // namespace SushiProfiler
