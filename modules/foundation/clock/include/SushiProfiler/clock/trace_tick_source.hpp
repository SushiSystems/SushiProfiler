/**************************************************************************/
/* trace_tick_source.hpp                                                  */
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
 * @file trace_tick_source.hpp
 * @brief Declares the tick source that reads the processor's counter or the steady clock.
 * @author Mustafa Garip
 * @see modules/foundation/clock/README.md
 */

#pragma once

#include <SushiProfiler/clock/calibration.hpp>
#include <SushiProfiler/clock/tick_source.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstdint>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Stamps with TraceClock ticks when the counter is invariant, else steady nanoseconds.
     *
     * The choice is made once, at construction.
     */
    class SUSHIPROFILER_API TraceTickSource final : public TickSource
    {
        public:
            /// @brief Asks the processor once whether its counter is invariant.
            TraceTickSource() noexcept;

            /// @brief Returns a counter reading, or steady nanoseconds without an invariant one.
            [[nodiscard]] std::uint64_t now() const noexcept override;

            /// @brief Returns TraceClock::sample(), or one steady reading as both halves.
            [[nodiscard]] CalibrationPoint sample() const noexcept override;

            /// @brief Returns whether the stamps are counter ticks.
            [[nodiscard]] bool ticks_are_tsc() const noexcept override;

        private:
            /// @brief Whether the counter is invariant, read at construction.
            bool use_tsc_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
