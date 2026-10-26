/**************************************************************************/
/* window.cpp                                                             */
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
 * @file window.cpp
 * @brief Places each period in its cycle and moves the collector's gate around its mark.
 * @author Mustafa Garip
 * @see modules/session/window/README.md
 */

#include <SushiProfiler/window/window.hpp>

#include <SushiProfiler/collector/collector.hpp>

namespace SushiProfiler
{
    Window::Window(const WindowSchedule& schedule) noexcept : schedule_(schedule)
    {

    }

    WindowPhase Window::phase_of(std::uint64_t index) const noexcept
    {
        if (index < schedule_.skip)
            return WindowPhase::Skipped;
        if (schedule_.active_all)
        {
            const std::uint64_t since_skip = index - schedule_.skip;
            if (since_skip < schedule_.wait)
                return WindowPhase::Waiting;
            if (since_skip < static_cast<std::uint64_t>(schedule_.wait) + schedule_.warmup)
                return WindowPhase::Warmup;
            return WindowPhase::Active;
        }
        const std::uint64_t cycle = static_cast<std::uint64_t>(schedule_.wait) +
                                    schedule_.warmup + schedule_.active;
        const std::uint64_t offset = index - schedule_.skip;
        if (schedule_.repeat != 0U && offset / cycle >= schedule_.repeat)
            return WindowPhase::Done;
        const std::uint64_t place = offset % cycle;
        if (place < schedule_.wait)
            return WindowPhase::Waiting;
        if (place < static_cast<std::uint64_t>(schedule_.wait) + schedule_.warmup)
            return WindowPhase::Warmup;
        return WindowPhase::Active;
    }

    bool Window::records(std::uint64_t index) const noexcept
    {
        const WindowPhase phase = phase_of(index);
        return phase == WindowPhase::Warmup || phase == WindowPhase::Active;
    }

    void Window::before_mark(const PeriodTimeline& timeline, std::uint64_t index) noexcept
    {
        static_cast<void>(timeline);
        if (records(index))
            Collector::instance().resume();
    }

    void Window::after_mark(const PeriodTimeline& timeline, std::uint64_t index) noexcept
    {
        static_cast<void>(timeline);
        if (!records(index))
            Collector::instance().pause();
    }
} // namespace SushiProfiler
