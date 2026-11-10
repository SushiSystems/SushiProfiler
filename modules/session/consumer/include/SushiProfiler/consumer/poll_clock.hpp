/**************************************************************************/
/* poll_clock.hpp                                                         */
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
 * @file poll_clock.hpp
 * @brief Declares the port a consumer waits between drains through.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#pragma once

#include <chrono>

namespace SushiProfiler
{
    /**
     * @brief Paces the consumer's thread: how long it waits between drains and what ends a wait.
     *
     * One thread calls wait(); any thread may call wake(), stop() and rearm(). Every
     * implementation keeps the promises the contract helpers of
     * tests/session/consumer/test_poll_clock.cpp state.
     */
    class PollClock
    {
        public:
            virtual ~PollClock() = default;

            /**
             * @brief Waits up to @p limit, or less when wake() or stop() is called.
             *
             * Clears a pending wake(). Returns false once stop() was called and rearm() has not
             * followed, so the caller leaves its loop.
             */
            [[nodiscard]] virtual bool wait(std::chrono::milliseconds limit) = 0;

            /// @brief Ends the current wait, or the next one when none runs, with a true result.
            virtual void wake() = 0;

            /// @brief Ends the current wait and makes every wait return false until rearm().
            virtual void stop() = 0;

            /// @brief Undoes stop(), so the thread can wait again.
            virtual void rearm() = 0;

        protected:
            PollClock() = default;
            PollClock(const PollClock&) = default;
            PollClock& operator=(const PollClock&) = default;
            PollClock(PollClock&&) = default;
            PollClock& operator=(PollClock&&) = default;
    };
} // namespace SushiProfiler
