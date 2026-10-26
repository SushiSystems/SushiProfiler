/**************************************************************************/
/* period_timeline.hpp                                                    */
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
 * @file period_timeline.hpp
 * @brief Declares a named timeline whose marks divide a run into numbered periods.
 * @author Mustafa Garip
 * @see modules/capture/periods/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/periods/period_observer.hpp>

#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>

namespace SushiProfiler
{
    /**
     * @brief Divides a run into periods, such as training steps or frames, numbered from 0.
     *
     * Each mark opens the next period and returns its index. The index advances whether or not
     * the collector records, and every recorded mark carries it, so a source that stamps its own
     * data with the index agrees with the record even when marks were closed out or dropped.
     */
    class PeriodTimeline final
    {
        public:
            /// @brief The value of `current()` before the first mark.
            static constexpr std::uint64_t K_NO_PERIOD = ~std::uint64_t{0U};

            /// @brief Builds the timeline @p name, interning it, with @p budget_ns per period or 0.
            explicit PeriodTimeline(std::string_view name, std::uint64_t budget_ns = 0U);

            PeriodTimeline(const PeriodTimeline&) = delete;
            PeriodTimeline& operator=(const PeriodTimeline&) = delete;
            PeriodTimeline(PeriodTimeline&&) = delete;
            PeriodTimeline& operator=(PeriodTimeline&&) = delete;
            ~PeriodTimeline() = default;

            /// @brief Opens the next period with the timeline's budget and returns its index.
            std::uint64_t mark() noexcept;

            /// @brief Opens the next period with budget @p budget_ns, 0 for none; returns its index.
            std::uint64_t mark(std::uint64_t budget_ns) noexcept;

            /// @brief Returns the index of the open period, or K_NO_PERIOD before the first mark.
            [[nodiscard]] std::uint64_t current() const noexcept;

            /**
             * @brief Attaches @p key = @p value to the open period, if the collector records.
             *
             * Interns both strings under a lock, so it is not for a real-time thread.
             */
            void tag(std::string_view key, std::string_view value);

            /**
             * @brief Makes @p observer hear every later mark; null removes the one installed.
             * @pre @p observer outlives its installation.
             */
            void observe(PeriodObserver* observer) noexcept;

            /// @brief Returns the timeline's interned name.
            [[nodiscard]] NameID id() const noexcept;

            /// @brief Returns the timeline's name.
            [[nodiscard]] std::string_view name() const noexcept;

        private:
            /// @brief Writes the mark of period @p index, and its budget, if the gate is open.
            void write_mark(std::uint64_t index, std::uint64_t budget_ns) const noexcept;

            /// @brief The timeline's name.
            std::string name_;

            /// @brief The interned name every event of the timeline carries.
            NameID id_;

            /// @brief The budget `mark()` gives a period, or 0.
            std::uint64_t budget_ns_;

            /// @brief The number of marks so far.
            std::atomic<std::uint64_t> marks_{0U};

            /// @brief The observer of the marks, or null.
            std::atomic<PeriodObserver*> observer_{nullptr};
    };
} // namespace SushiProfiler
