/**************************************************************************/
/* span_sink.hpp                                                          */
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
 * @file span_sink.hpp
 * @brief Declares where a source puts a period's spans, refusing what the record cannot hold.
 * @author Mustafa Garip
 * @see modules/session/sources/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/record/period_record.hpp>
#include <SushiProfiler/record/span_table.hpp>
#include <SushiProfiler/sites/name_table.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Adds a source's spans and their attributes to one period's span table.
     *
     * A span on a track or with a name the record does not hold, or whose times are out of
     * order, is refused and counted, as is an attribute on such a key or of another kind than
     * its column, so the record stays one that read_record accepts.
     */
    class SUSHIPROFILER_API SpanSink final
    {
        public:
            /**
             * @brief Builds a sink over @p table that checks ids against @p names and @p tracks
             *        and sets facts on @p facts, the period's.
             */
            SpanSink(SpanTable& table, std::vector<Fact>& facts, const NameTable& names,
                     const std::vector<std::string>& tracks) noexcept;

            /// @brief Builds a sink with no facts: set_fact() refuses every call.
            SpanSink(SpanTable& table, const NameTable& names,
                     const std::vector<std::string>& tracks) noexcept;

            SpanSink(const SpanSink&) = delete;
            SpanSink& operator=(const SpanSink&) = delete;
            SpanSink(SpanSink&&) = delete;
            SpanSink& operator=(SpanSink&&) = delete;
            ~SpanSink() = default;

            /**
             * @brief Appends @p span and returns its row, or refuses it.
             *
             * Times must run `queued_ns <= start_ns <= end_ns`; a source that does not know when
             * the work was queued gives its start time.
             */
            [[nodiscard]] std::optional<std::size_t> append(const Span& span);

            /// @brief Sets integer attribute @p key of @p row; returns whether it was accepted.
            bool set_integer(std::size_t row, NameID key, std::int64_t value);

            /// @brief Sets real attribute @p key of @p row; returns whether it was accepted.
            bool set_real(std::size_t row, NameID key, double value);

            /// @brief Sets name attribute @p key of @p row; returns whether it was accepted.
            bool set_name(std::size_t row, NameID key, NameID value);

            /**
             * @brief Sets fact @p name of the period to @p value, replacing one of that name.
             * @return false, counted as a refusal, for an empty name or a sink with no facts.
             */
            [[nodiscard]] bool set_fact(std::string_view name, std::string_view value);

            /// @brief Returns how many spans, attributes and facts were refused.
            [[nodiscard]] std::uint64_t refused() const noexcept;

            /// @brief Counts @p count spans the source dropped because their period had closed.
            void drop_late(std::uint64_t count) noexcept;

            /// @brief Returns how many spans drop_late() counted.
            [[nodiscard]] std::uint64_t late() const noexcept;

        private:
            /// @brief Returns whether @p id is an interned name.
            [[nodiscard]] bool known_name(NameID id) const;

            /// @brief Returns whether an attribute of @p kind on @p key may be set on @p row.
            bool accepts(std::size_t row, NameID key, AttributeKind kind);

            /// @brief The period's spans.
            SpanTable& table_;

            /// @brief The period's facts, or null for a sink built without them.
            std::vector<Fact>* facts_;

            /// @brief The session's names.
            const NameTable& names_;

            /// @brief The record's tracks.
            const std::vector<std::string>& tracks_;

            /// @brief Spans and attributes refused so far.
            std::uint64_t refused_ = 0U;

            /// @brief Spans the source dropped as late.
            std::uint64_t late_ = 0U;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
