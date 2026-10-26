/**************************************************************************/
/* live_channel.hpp                                                       */
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
 * @file live_channel.hpp
 * @brief Declares the channel that carries closed periods from the consumer to live readers.
 * @author Mustafa Garip
 * @see modules/session/live/README.md
 */

#pragma once

#include <SushiProfiler/live/closed_period.hpp>
#include <SushiProfiler/version/export.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Holds the newest closed period and a ring of the last ones.
     *
     * The consumer publishes; any thread reads. A reader keeps the periods it holds through ring
     * wraps, new runs and the end of the session. `latest()` takes a short library lock (atomic
     * load of a shared pointer). The history mutex only moves pointers.
     */
    class SUSHIPROFILER_API LiveChannel final
    {
        public:
            /// @brief Builds a channel that remembers the last @p history_capacity periods, at least 1.
            explicit LiveChannel(std::size_t history_capacity);

            LiveChannel(const LiveChannel&) = delete;
            LiveChannel& operator=(const LiveChannel&) = delete;
            LiveChannel(LiveChannel&&) = delete;
            LiveChannel& operator=(LiveChannel&&) = delete;
            ~LiveChannel() = default;

            /// @brief Empties the history and the latest pointer and returns the new run number.
            std::uint64_t begin_run();

            /// @brief Returns the current run number; 0 before the first begin_run().
            [[nodiscard]] std::uint64_t run() const noexcept;

            /// @brief Stores @p period as the newest; the consumer thread calls it.
            void publish(std::shared_ptr<const ClosedPeriod> period);

            /// @brief Returns the newest period, or null before the first of the run.
            [[nodiscard]] std::shared_ptr<const ClosedPeriod> latest() const noexcept;

            /// @brief Returns the last @p count periods, at most the capacity, oldest first.
            [[nodiscard]] std::vector<std::shared_ptr<const ClosedPeriod>> history(
                std::size_t count) const;

            /// @brief Returns whether a LiveView exists, so periods are worth publishing.
            [[nodiscard]] bool watched() const noexcept;

            /// @brief Counts one more viewer.
            void attach() noexcept;

            /// @brief Counts one viewer less.
            void detach() noexcept;

            /// @brief Returns how many periods were published since begin_run().
            [[nodiscard]] std::uint64_t published() const noexcept;

        private:
            std::size_t capacity_;                      /**< The ring's size. */
            mutable std::mutex mutex_;                  /**< Guards ring_, head_, size_, latest_. */
            std::vector<std::shared_ptr<const ClosedPeriod>> ring_; /**< The history. */
            std::size_t head_ = 0U;                     /**< Where the next period goes. */
            std::size_t size_ = 0U;                     /**< Periods held. */
            std::shared_ptr<const ClosedPeriod> latest_; /**< The newest; atomic_load and store. */
            std::atomic<std::uint64_t> published_{0U};  /**< Periods published in this run. */
            std::atomic<std::uint64_t> run_{0U};        /**< The run number. */
            std::atomic<std::uint32_t> viewers_{0U};    /**< LiveViews alive. */
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
