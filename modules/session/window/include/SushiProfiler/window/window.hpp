/**************************************************************************/
/* window.hpp                                                             */
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
 * @file window.hpp
 * @brief Declares the capture window, which opens the gate for the scheduled periods only.
 * @author Mustafa Garip
 * @see modules/session/window/README.md
 */

#pragma once

#include <SushiProfiler/config/profiler_config.hpp>
#include <SushiProfiler/periods/period_observer.hpp>

#include <cstdint>

namespace SushiProfiler
{
    /// @brief Where a period falls in the window's schedule.
    enum class WindowPhase : std::uint8_t
    {
        Skipped, /**< Before the first cycle; not recorded. */
        Waiting, /**< At the start of a cycle; not recorded. */
        Warmup,  /**< Recorded and marked warm-up. */
        Active,  /**< Recorded. */
        Done     /**< After the last cycle; not recorded. */
    };

    /**
     * @brief Follows one timeline and opens the collector's gate for its recorded periods.
     *
     * Before the mark that opens a warm-up or active period it resumes the collector, so the mark
     * is written; after the mark that opens any other period it pauses it, so the period before
     * still gets its end. The session opens the collector paused.
     */
    class Window final : public PeriodObserver
    {
        public:
            /// @brief Builds a window over @p schedule; `schedule.active` is at least 1.
            explicit Window(const WindowSchedule& schedule) noexcept;

            /// @brief Returns the phase of period @p index.
            [[nodiscard]] WindowPhase phase_of(std::uint64_t index) const noexcept;

            /// @brief Returns whether period @p index is recorded.
            [[nodiscard]] bool records(std::uint64_t index) const noexcept;

            /// @brief Resumes the collector when period @p index is recorded.
            void before_mark(const PeriodTimeline& timeline,
                             std::uint64_t index) noexcept override;

            /// @brief Pauses the collector when period @p index is not recorded.
            void after_mark(const PeriodTimeline& timeline,
                            std::uint64_t index) noexcept override;

        private:
            /// @brief The schedule.
            WindowSchedule schedule_;
    };
} // namespace SushiProfiler
