/**************************************************************************/
/* calibration.hpp                                                        */
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
 * @file calibration.hpp
 * @brief Declares a pairing of clock ticks with steady time and the line through two of them.
 * @author Mustafa Garip
 * @see modules/foundation/clock/README.md
 */

#pragma once

#include <cmath>
#include <cstdint>

namespace SushiProfiler
{
    /// @brief One tick reading paired with the steady-clock reading taken beside it.
    struct CalibrationPoint
    {
        std::uint64_t ticks = 0U;     /**< The tick reading. */
        std::uint64_t steady_ns = 0U; /**< The steady-clock nanoseconds read beside it. */
    };

    /**
     * @brief Turns ticks into steady nanoseconds along the line through two calibration points.
     *
     * Readings before the first point or after the second are extrapolated along the same line.
     * Two points with equal ticks keep one nanosecond per tick.
     */
    class Calibration
    {
        public:
            /// @brief Builds the identity: one nanosecond per tick from zero.
            Calibration() noexcept = default;

            /// @brief Builds the line from @p begin through @p end.
            Calibration(const CalibrationPoint& begin, const CalibrationPoint& end) noexcept
                : begin_(begin)
            {
                if (end.ticks != begin.ticks)
                {
                    const double steady_span = static_cast<double>(
                        static_cast<std::int64_t>(end.steady_ns - begin.steady_ns));
                    const double tick_span =
                        static_cast<double>(static_cast<std::int64_t>(end.ticks - begin.ticks));
                    nanoseconds_per_tick_ = steady_span / tick_span;
                }
            }

            /// @brief Returns the steady nanoseconds that @p ticks falls on.
            [[nodiscard]] std::uint64_t to_nanoseconds(std::uint64_t ticks) const noexcept
            {
                // Unsigned wrap keeps the offset exact for readings on either side of begin_.
                const double tick_offset =
                    static_cast<double>(static_cast<std::int64_t>(ticks - begin_.ticks));
                const std::int64_t ns_offset = std::llround(tick_offset * nanoseconds_per_tick_);
                return begin_.steady_ns + static_cast<std::uint64_t>(ns_offset);
            }

            /// @brief Returns the slope of the line.
            [[nodiscard]] double nanoseconds_per_tick() const noexcept
            {
                return nanoseconds_per_tick_;
            }

        private:
            /// @brief The point the line starts from.
            CalibrationPoint begin_;

            /// @brief Nanoseconds one tick lasts.
            double nanoseconds_per_tick_ = 1.0;
    };
} // namespace SushiProfiler
