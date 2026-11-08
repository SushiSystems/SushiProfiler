/**************************************************************************/
/* memory_source.hpp                                                      */
/**************************************************************************/
/*                          This file is part of:                         */
/*                              SushiProfiler                             */
/*              https://github.com/sushisystems/sushiprofiler             */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file memory_source.hpp
 * @brief Declares the in-memory source, which hands over the work queued into it.
 * @author Mustafa Garip
 * @see modules/session/sources/README.md
 */

#pragma once

#include <SushiProfiler/sources/source.hpp>
#include <SushiProfiler/sources/source_context.hpp>
#include <SushiProfiler/sources/span_sink.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief One piece of work queued into a MemorySource, timed on the steady clock.
    struct MemoryWork
    {
        std::string track;            /**< The track it ran on. */
        std::string name;             /**< The span's name. */
        std::uint64_t queued_ns = 0U; /**< When it was queued, in steady nanoseconds. */
        std::uint64_t start_ns = 0U;  /**< When it started, in steady nanoseconds. */
        std::uint64_t end_ns = 0U;    /**< When it ended, in steady nanoseconds. */
    };

    /**
     * @brief Hands over queued work as spans of the period its start lies in.
     *
     * collect() adds each piece that started inside the period, on the record's clock, drops as
     * late each piece that started before it, and keeps the rest for a later period. It sets the
     * facts state_fact() gave on every period. Any thread may queue while the consumer collects.
     */
    class SUSHIPROFILER_API MemorySource final : public Source
    {
        public:
            /// @brief Builds a source named @p name that states @p max_delay_ns.
            explicit MemorySource(std::string name, std::uint64_t max_delay_ns = 0U);

            /// @brief Makes begin() refuse with @p error; an empty one lets it start.
            void refuse_with(std::string error);

            /// @brief Queues @p work for the period its start lies in.
            void queue(MemoryWork work);

            /// @brief Sets fact @p name to @p value on every period collected.
            void state_fact(std::string name, std::string value);

            /// @brief Returns the name given at construction.
            [[nodiscard]] std::string_view name() const noexcept override;

            /// @brief Keeps @p context, or refuses with the error refuse_with() gave.
            [[nodiscard]] SourceStatus begin(SourceContext& context) override;

            /// @brief Returns the delay given at construction.
            [[nodiscard]] std::uint64_t max_delay_ns() const noexcept override;

            /// @brief Adds the work that started inside @p period and drops what started before.
            void collect(const PeriodSpan& period, SpanSink& sink) override;

            /// @brief Forgets the context begin() kept.
            void end() override;

            /// @brief Returns the periods collect() received, in order.
            [[nodiscard]] std::vector<PeriodSpan> collected() const;

            /// @brief Returns how many times end() ran.
            [[nodiscard]] std::size_t ends() const;

        private:
            /// @brief One fact set on every period.
            struct StatedFact
            {
                std::string name;  /**< The fact's name. */
                std::string value; /**< Its value. */
            };

            /// @brief The name.
            std::string name_;

            /// @brief The delay stated.
            std::uint64_t max_delay_ns_;

            /// @brief Guards everything below.
            mutable std::mutex mutex_;

            /// @brief Why begin() refuses, or empty.
            std::string refusal_;

            /// @brief The context begin() received, until end().
            SourceContext* context_ = nullptr;

            /// @brief The work not handed over yet.
            std::vector<MemoryWork> pending_;

            /// @brief The facts set on every period.
            std::vector<StatedFact> facts_;

            /// @brief The periods collect() received.
            std::vector<PeriodSpan> collected_;

            /// @brief How many times end() ran.
            std::size_t ends_ = 0U;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
