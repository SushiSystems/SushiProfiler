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
     * @brief Hands the session spans measured outside the capture path, such as device work.
     *
     * The session calls begin() once at start, collect() once per closed period on the consumer
     * thread, and end() once at stop. A source that receives its data on another thread queues
     * it and hands it over in collect().
     */
    class Source
    {
        public:
            virtual ~Source() = default;

            /// @brief Returns the name the registry knows this source by.
            [[nodiscard]] virtual std::string_view name() const noexcept = 0;

            /// @brief Adds the source's tracks and keys through @p context, or says why it cannot.
            [[nodiscard]] virtual SourceStatus begin(SourceContext& context) = 0;

            /// @brief Adds every span tagged with @p period to @p sink.
            virtual void collect(std::uint64_t period, SpanSink& sink) = 0;

            /// @brief Releases what begin() acquired.
            virtual void end() = 0;
    };
} // namespace SushiProfiler
