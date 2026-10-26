/**************************************************************************/
/* span_table.hpp                                                         */
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
 * @file span_table.hpp
 * @brief Declares a period's spans as columns, with one column per attribute key.
 * @author Mustafa Garip
 * @see modules/record/record/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief Names a track a source reports spans on, such as one device queue.
    using TrackID = std::uint32_t;

    /// @brief One span a source reports, its times in nanoseconds on the record's clock.
    struct Span
    {
        std::uint64_t queued_ns = 0U;       /**< When the work was submitted. */
        std::uint64_t start_ns = 0U;        /**< When it started. */
        std::uint64_t end_ns = 0U;          /**< When it ended. */
        TrackID track = 0U;                 /**< The track it ran on. */
        NameID name = 0U;                   /**< What ran. */
        std::uint32_t submit_thread = 0U;   /**< The thread index that submitted it. */
        std::uint32_t uncertainty_ns = 0U;  /**< The source's clock-conversion uncertainty. */
    };

    /// @brief Names what an attribute column holds.
    enum class AttributeKind : std::uint8_t
    {
        Integer, /**< Signed whole numbers. */
        Real,    /**< Doubles. */
        Name     /**< Interned names, held as NameIDs. */
    };

    /// @brief One attribute key's values over a period's spans; a span may lack it.
    struct SUSHIPROFILER_API AttributeColumn
    {
        NameID key = 0U;                        /**< The attribute's interned key. */
        AttributeKind kind = AttributeKind::Integer; /**< What the column holds. */
        std::vector<std::int64_t> integers;     /**< Integer and Name values, one per span. */
        std::vector<double> reals;              /**< Real values, one per span. */
        std::vector<std::uint8_t> present;      /**< 1 where the span has the attribute. */

        /// @brief Returns whether key, kind and every value are equal.
        [[nodiscard]] bool operator==(const AttributeColumn& other) const;
    };

    /**
     * @brief Holds a period's spans as aligned columns.
     *
     * Every column, attributes included, has one entry per span. A column added after spans
     * exist marks them absent, and a span added after columns exist is absent from each.
     */
    class SUSHIPROFILER_API SpanTable
    {
        public:
            /// @brief Appends @p span and returns its row.
            std::size_t append(const Span& span);

            /// @brief Sets integer attribute @p key of row @p row.
            void set_integer(std::size_t row, NameID key, std::int64_t value);

            /// @brief Sets real attribute @p key of row @p row.
            void set_real(std::size_t row, NameID key, double value);

            /// @brief Sets name attribute @p key of row @p row to the interned @p value.
            void set_name(std::size_t row, NameID key, NameID value);

            /// @brief Returns the number of spans.
            [[nodiscard]] std::size_t size() const noexcept;

            /// @brief Returns span @p row.
            [[nodiscard]] Span span(std::size_t row) const;

            /// @brief Returns the column of @p key, or null.
            [[nodiscard]] const AttributeColumn* attribute(NameID key) const noexcept;

            /// @brief Returns every attribute column, in the order they were first set.
            [[nodiscard]] const std::vector<AttributeColumn>& attributes() const noexcept;

            /// @brief Adds a column read from a file; its vectors must have one entry per span.
            void add_column(AttributeColumn column);

            /// @brief Returns whether every column is equal.
            [[nodiscard]] bool operator==(const SpanTable& other) const;

        private:
            /// @brief Returns the column of @p key, adding one of @p kind when there is none.
            AttributeColumn& column(NameID key, AttributeKind kind);

            std::vector<std::uint64_t> queued_ns_;      /**< Span::queued_ns per span. */
            std::vector<std::uint64_t> start_ns_;       /**< Span::start_ns per span. */
            std::vector<std::uint64_t> end_ns_;         /**< Span::end_ns per span. */
            std::vector<TrackID> track_;                /**< Span::track per span. */
            std::vector<NameID> name_;                  /**< Span::name per span. */
            std::vector<std::uint32_t> submit_thread_;  /**< Span::submit_thread per span. */
            std::vector<std::uint32_t> uncertainty_ns_; /**< Span::uncertainty_ns per span. */
            std::vector<AttributeColumn> attributes_;   /**< The attribute columns. */
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
