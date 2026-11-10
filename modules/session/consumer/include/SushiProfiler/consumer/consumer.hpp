/**************************************************************************/
/* consumer.hpp                                                           */
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
 * @file consumer.hpp
 * @brief Declares the thread that drains the rings into the assembler at an adaptive interval.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#pragma once

#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/collector/drain_batch.hpp>
#include <SushiProfiler/consumer/period_assembler.hpp>
#include <SushiProfiler/consumer/poll_clock.hpp>
#include <SushiProfiler/consumer/steady_poll_clock.hpp>
#include <SushiProfiler/live/live_channel.hpp>
#include <SushiProfiler/version/export.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief The shortest wait between drains.
    inline constexpr std::chrono::milliseconds K_FASTEST_POLL{1};

    /// @brief The longest wait between drains.
    inline constexpr std::chrono::milliseconds K_SLOWEST_POLL{1000};

    /**
     * @brief Returns the wait before the next drain, from the last wait and the fullest ring.
     *
     * Over half full gives the fastest poll; under a quarter doubles the wait up to the slowest;
     * in between keeps it.
     */
    [[nodiscard]] SUSHIPROFILER_API std::chrono::milliseconds next_poll(
        std::chrono::milliseconds current, double fullest) noexcept;

    /// @brief The slowest wait between drains while a live view exists, unless configured.
    inline constexpr std::chrono::milliseconds K_DEFAULT_LIVE_POLL{4};

    /// @brief Like next_poll(current, fullest) with @p slowest in place of K_SLOWEST_POLL.
    [[nodiscard]] SUSHIPROFILER_API std::chrono::milliseconds next_poll(
        std::chrono::milliseconds current, double fullest,
        std::chrono::milliseconds slowest) noexcept;

    /// @brief Drains the collector into an assembler on its own thread until stopped.
    class SUSHIPROFILER_API Consumer final
    {
        public:
            /// @brief Builds a consumer of @p collector into @p assembler; both outlive it.
            Consumer(Collector& collector, PeriodAssembler& assembler) noexcept;

            /**
             * @brief Builds a consumer that waits at most @p live_slowest between drains while
             *        @p live has a viewer; all three outlive it.
             */
            Consumer(Collector& collector, PeriodAssembler& assembler, const LiveChannel& live,
                     std::chrono::milliseconds live_slowest) noexcept;

            /**
             * @brief Like the live constructor, with the waits between drains made through
             *        @p clock in place of the machine's steady clock; @p clock outlives it too.
             */
            Consumer(Collector& collector, PeriodAssembler& assembler, const LiveChannel& live,
                     std::chrono::milliseconds live_slowest, PollClock& clock) noexcept;

            Consumer(const Consumer&) = delete;
            Consumer& operator=(const Consumer&) = delete;
            Consumer(Consumer&&) = delete;
            Consumer& operator=(Consumer&&) = delete;

            /// @brief Stops the thread if it runs.
            ~Consumer();

            /// @brief Starts the thread.
            void start();

            /// @brief Drains once more, finishes the assembler and joins; does nothing if idle.
            void stop();

            /// @brief Returns how many drains the thread has done, the final one included.
            [[nodiscard]] std::uint64_t drains() const noexcept;

            /// @brief Ends the current wait early, so a viewer that just appeared is served at once.
            void wake();

        private:
            /// @brief Returns the slowest wait now: the live ceiling while a view exists.
            [[nodiscard]] std::chrono::milliseconds slowest_poll() const noexcept;

            /// @brief Polls until stopped.
            void run();

            /// @brief Drains the collector into the assembler; returns the fullest ring's fill.
            double drain_once();

            /// @brief The collector drained.
            Collector& collector_;

            /// @brief Where the events go.
            PeriodAssembler& assembler_;

            /// @brief The channel whose viewers speed the poll up, or null.
            const LiveChannel* live_ = nullptr;

            /// @brief The slowest wait while live_ has a viewer.
            std::chrono::milliseconds live_slowest_{K_DEFAULT_LIVE_POLL};

            /// @brief The batch drains move events through, kept to reuse its memory.
            DrainBatch batch_;

            /// @brief The draining thread.
            std::thread thread_;

            /// @brief Guards thread_ against two starts or stops at once.
            std::mutex mutex_;

            /// @brief The clock used when none was given: real time.
            SteadyPollClock own_clock_;

            /// @brief What the thread waits through between drains.
            PollClock* clock_ = &own_clock_;

            /// @brief Drains done; read from other threads.
            std::atomic<std::uint64_t> drains_{0U};
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
