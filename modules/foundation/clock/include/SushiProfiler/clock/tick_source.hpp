/**************************************************************************/
/* tick_source.hpp                                                        */
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
 * @file tick_source.hpp
 * @brief Declares the port a collector reads its event stamps and calibration points through.
 * @author Mustafa Garip
 * @see modules/foundation/clock/README.md
 */

#pragma once

#include <SushiProfiler/clock/calibration.hpp>

#include <cstdint>

namespace SushiProfiler
{
    /**
     * @brief Gives the stamps a collector writes into events and the points that calibrate them.
     *
     * A stamp is a tick when `ticks_are_tsc()` holds and steady nanoseconds when it does not.
     * Every implementation keeps the promises of `TickSourceContract` in
     * tests/foundation/clock/test_tick_source.cpp.
     */
    class TickSource
    {
        public:
            virtual ~TickSource() = default;

            /// @brief Returns the stamp of now; never less than an earlier stamp.
            [[nodiscard]] virtual std::uint64_t now() const noexcept = 0;

            /// @brief Returns a stamp paired with the steady nanoseconds read beside it.
            [[nodiscard]] virtual CalibrationPoint sample() const noexcept = 0;

            /// @brief Returns whether the stamps are time-stamp-counter ticks.
            [[nodiscard]] virtual bool ticks_are_tsc() const noexcept = 0;

        protected:
            TickSource() = default;
            TickSource(const TickSource&) = default;
            TickSource& operator=(const TickSource&) = default;
            TickSource(TickSource&&) = default;
            TickSource& operator=(TickSource&&) = default;
    };
} // namespace SushiProfiler
