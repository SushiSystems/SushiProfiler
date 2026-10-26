/**************************************************************************/
/* record_clock.hpp                                                       */
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
 * @file record_clock.hpp
 * @brief Declares the conversion of an event's ticks and steady times onto the record's clock.
 * @author Mustafa Garip
 * @see modules/session/fold/README.md
 */

#pragma once

#include <SushiProfiler/clock/calibration.hpp>
#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstdint>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief Moves event times onto the record's clock, nanoseconds since the session's origin.
    class SUSHIPROFILER_API RecordClock final
    {
        public:
            /**
             * @brief Builds a clock from the drain's @p calibration.
             * @param origin_steady_ns The steady time the record counts from.
             * @param ticks_are_tsc Whether event ticks are TSC; otherwise they are steady ns.
             */
            RecordClock(const Calibration& calibration, std::uint64_t origin_steady_ns,
                        bool ticks_are_tsc) noexcept;

            /// @brief Returns @p ticks on the record's clock; a time before the origin is 0.
            [[nodiscard]] std::uint64_t from_ticks(std::uint64_t ticks) const noexcept;

            /// @brief Returns @p steady_ns on the record's clock; a time before the origin is 0.
            [[nodiscard]] std::uint64_t from_steady(std::uint64_t steady_ns) const noexcept;

            /// @brief Returns @p event with each time field moved onto the record's clock.
            [[nodiscard]] TraceEvent to_record(const TraceEvent& event) const noexcept;

        private:
            /// @brief The mapping from ticks to steady time.
            Calibration calibration_;

            /// @brief The steady time the record counts from.
            std::uint64_t origin_steady_ns_;

            /// @brief Whether ticks are TSC.
            bool ticks_are_tsc_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
