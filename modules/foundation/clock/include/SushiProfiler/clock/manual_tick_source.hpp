/**************************************************************************/
/* manual_tick_source.hpp                                                 */
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
 * @file manual_tick_source.hpp
 * @brief Declares the in-memory tick source whose time moves only when its owner advances it.
 * @author Mustafa Garip
 * @see modules/foundation/clock/README.md
 */

#pragma once

#include <SushiProfiler/clock/calibration.hpp>
#include <SushiProfiler/clock/tick_source.hpp>
#include <SushiProfiler/version/export.hpp>

#include <atomic>
#include <cstdint>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Stamps with steady nanoseconds that change only through advance().
     *
     * Its stamps are nanoseconds, so a sample pairs a stamp with itself. Any thread may read it
     * while one thread advances it.
     */
    class SUSHIPROFILER_API ManualTickSource final : public TickSource
    {
        public:
            /// @brief Starts the time at @p start_ns.
            explicit ManualTickSource(std::uint64_t start_ns = 0U) noexcept;

            /// @brief Moves the time forward by @p elapsed_ns.
            void advance(std::uint64_t elapsed_ns) noexcept;

            /// @brief Returns the time.
            [[nodiscard]] std::uint64_t now() const noexcept override;

            /// @brief Returns the time as both the stamp and the steady reading.
            [[nodiscard]] CalibrationPoint sample() const noexcept override;

            /// @brief Returns false: the stamps are nanoseconds.
            [[nodiscard]] bool ticks_are_tsc() const noexcept override;

        private:
            /// @brief The time in nanoseconds.
            std::atomic<std::uint64_t> now_ns_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
