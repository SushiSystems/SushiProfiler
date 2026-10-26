/**************************************************************************/
/* source_context.hpp                                                     */
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
 * @file source_context.hpp
 * @brief Declares what a starting source may add to the record: names, tracks and its clock.
 * @author Mustafa Garip
 * @see modules/session/sources/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/record/span_table.hpp>
#include <SushiProfiler/sites/name_table.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief Gives a starting source the record's names, its track table and its time origin.
    class SUSHIPROFILER_API SourceContext final
    {
        public:
            /**
             * @brief Builds a context over the session's @p names and @p tracks.
             * @param origin_steady_ns The steady time the record counts from.
             */
            SourceContext(NameTable& names, std::vector<std::string>& tracks,
                          std::uint64_t origin_steady_ns) noexcept;

            SourceContext(const SourceContext&) = delete;
            SourceContext& operator=(const SourceContext&) = delete;
            SourceContext(SourceContext&&) = delete;
            SourceContext& operator=(SourceContext&&) = delete;
            ~SourceContext() = default;

            /// @brief Returns the id of @p text, for span names and attribute keys and values.
            [[nodiscard]] NameID intern(std::string_view text);

            /// @brief Returns the track named @p name, adding it on first use.
            [[nodiscard]] TrackID add_track(std::string_view name);

            /// @brief Returns @p steady_ns on the record's clock; a time before the origin is 0.
            [[nodiscard]] std::uint64_t to_record_ns(std::uint64_t steady_ns) const noexcept;

        private:
            /// @brief The session's name table.
            NameTable& names_;

            /// @brief The record's track names, by TrackID.
            std::vector<std::string>& tracks_;

            /// @brief The steady time the record counts from.
            std::uint64_t origin_steady_ns_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
