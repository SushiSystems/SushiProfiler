/**************************************************************************/
/* manual_poll_clock.hpp                                                  */
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
 * @file manual_poll_clock.hpp
 * @brief Declares the in-memory poll clock whose waits end only when its owner advances it.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#pragma once

#include <SushiProfiler/consumer/poll_clock.hpp>
#include <SushiProfiler/version/export.hpp>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Keeps a time that moves only through advance(), and ends a wait once it passes.
     *
     * Each wait() notes its limit, so a test reads what the consumer asked for. One thread waits;
     * another advances, wakes and reads.
     */
    class SUSHIPROFILER_API ManualPollClock final : public PollClock
    {
        public:
            /// @brief Notes @p limit and waits until advance() passes it, wake() or stop().
            [[nodiscard]] bool wait(std::chrono::milliseconds limit) override;

            /// @brief Ends the current wait, or the next one when none runs.
            void wake() override;

            /// @brief Ends the current wait; every wait returns false until rearm().
            void stop() override;

            /// @brief Lets the thread wait again.
            void rearm() override;

            /// @brief Moves the time forward by @p elapsed; a wait whose limit passed ends.
            void advance(std::chrono::milliseconds elapsed);

            /**
             * @brief Lets the consumer through one poll: waits until it waits, advances the time
             *        by the limit of that wait, and returns once it has drained and waits again.
             *
             * Nothing else may end the consumer's waits meanwhile; a wake() ends one without
             * this call, so await_waits() the drain it causes first.
             */
            void poll_once();

            /// @brief Blocks until wait() has been entered @p count times in all.
            void await_waits(std::size_t count);

            /// @brief Returns the limit of every wait entered so far, in order.
            [[nodiscard]] std::vector<std::chrono::milliseconds> limits() const;

        private:
            /// @brief Guards everything below.
            mutable std::mutex mutex_;

            /// @brief Ends a wait when time passes or a flag is set.
            std::condition_variable changed_;

            /// @brief Tells await_waits() that a wait was entered.
            std::condition_variable entered_;

            /// @brief The time advance() built up.
            std::chrono::milliseconds elapsed_{0};

            /// @brief The limit of every wait entered.
            std::vector<std::chrono::milliseconds> limits_;

            /// @brief Whether a thread is inside wait().
            bool waiting_ = false;

            /// @brief Whether stop() was called and rearm() has not followed.
            bool stopping_ = false;

            /// @brief Whether wake() was called since the last wait returned.
            bool woken_ = false;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
