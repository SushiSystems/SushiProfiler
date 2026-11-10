/**************************************************************************/
/* steady_poll_clock.hpp                                                  */
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
 * @file steady_poll_clock.hpp
 * @brief Declares the poll clock that waits on the machine's steady clock.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#pragma once

#include <SushiProfiler/consumer/poll_clock.hpp>
#include <SushiProfiler/version/export.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief Waits for real time on a condition variable; what a running session uses.
    class SUSHIPROFILER_API SteadyPollClock final : public PollClock
    {
        public:
            /// @brief Waits up to @p limit of real time, or less when woken or stopped.
            [[nodiscard]] bool wait(std::chrono::milliseconds limit) override;

            /// @brief Ends the current wait, or the next one when none runs.
            void wake() override;

            /// @brief Ends the current wait; every wait returns false until rearm().
            void stop() override;

            /// @brief Lets the thread wait again.
            void rearm() override;

        private:
            /// @brief Guards the two flags.
            std::mutex mutex_;

            /// @brief Ends a wait when a flag is set.
            std::condition_variable changed_;

            /// @brief Whether stop() was called and rearm() has not followed.
            bool stopping_ = false;

            /// @brief Whether wake() was called since the last wait returned.
            bool woken_ = false;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
