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
 * @file consumer.hpp
 * @brief Declares the thread that drains the rings into the assembler at an adaptive interval.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#pragma once

#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/collector/drain_batch.hpp>
#include <SushiProfiler/consumer/period_assembler.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

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
    [[nodiscard]] std::chrono::milliseconds next_poll(std::chrono::milliseconds current,
                                                      double fullest) noexcept;

    /// @brief Drains the collector into an assembler on its own thread until stopped.
    class Consumer final
    {
        public:
            /// @brief Builds a consumer of @p collector into @p assembler; both outlive it.
            Consumer(Collector& collector, PeriodAssembler& assembler) noexcept;

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

        private:
            /// @brief Polls until stopped.
            void run();

            /// @brief Drains the collector into the assembler; returns the fullest ring's fill.
            double drain_once();

            /// @brief The collector drained.
            Collector& collector_;

            /// @brief Where the events go.
            PeriodAssembler& assembler_;

            /// @brief The batch drains move events through, kept to reuse its memory.
            DrainBatch batch_;

            /// @brief The draining thread.
            std::thread thread_;

            /// @brief Guards stopping_.
            std::mutex mutex_;

            /// @brief Wakes the thread to stop.
            std::condition_variable wake_;

            /// @brief Whether stop() was called.
            bool stopping_ = false;
    };
} // namespace SushiProfiler
