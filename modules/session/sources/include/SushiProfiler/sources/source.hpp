/**************************************************************************/
/* source.hpp                                                             */
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
 * @file source.hpp
 * @brief Declares the interface a source of spans implements, and how its start can fail.
 * @author Mustafa Garip
 * @see modules/session/sources/README.md
 */

#pragma once

#include <SushiProfiler/sources/source_context.hpp>
#include <SushiProfiler/sources/span_sink.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace SushiProfiler
{
    /// @brief Whether a source started, or why not.
    struct SourceStatus
    {
        std::string error; /**< Why the source could not start, or empty. */

        /// @brief Returns whether the source started.
        [[nodiscard]] bool ok() const noexcept
        {
            return error.empty();
        }
    };

    /**
     * @brief The steady-clock span of one closed period, handed to Source::collect.
     *
     * A source places a piece of work in the period whose span holds the work's start time, so
     * it needs no handle to a timeline and no clock of its own.
     */
    struct PeriodSpan
    {
        std::uint64_t index = 0U;           /**< The period's index on its timeline. */
        std::uint64_t start_steady_ns = 0U; /**< The period's start, in steady-clock nanoseconds. */
        std::uint64_t end_steady_ns = 0U;   /**< The period's end, exclusive, in steady ns. */
    };

    /**
     * @brief Hands the session spans measured outside the capture path, such as device work.
     *
     * begin(), collect() and end() run on the consumer thread. The SourceContext that begin()
     * receives stays valid until end() returns, and collect() may use it without a lock. A source
     * fed from another thread queues the data and hands it over in collect().
     */
    class Source
    {
        public:
            virtual ~Source() = default;

            /// @brief Returns the name the registry knows this source by.
            [[nodiscard]] virtual std::string_view name() const noexcept = 0;

            /// @brief Adds the source's tracks and keys through @p context, or says why it cannot.
            [[nodiscard]] virtual SourceStatus begin(SourceContext& context) = 0;

            /**
             * @brief Adds to @p sink every span whose start lies inside @p period.
             *
             * Work that started before period.start_steady_ns belongs to a period that has
             * closed; the source calls SpanSink::drop_late() for it instead of adding it.
             */
            virtual void collect(const PeriodSpan& period, SpanSink& sink) = 0;

            /// @brief Releases what begin() acquired.
            virtual void end() = 0;
    };
} // namespace SushiProfiler
