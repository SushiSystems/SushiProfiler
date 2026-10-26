/**************************************************************************/
/* collector.hpp                                                          */
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
 * @file collector.hpp
 * @brief Declares the collector: the gate, the thread rings and the drain of every record.
 * @author Mustafa Garip
 * @see modules/capture/collector/README.md
 */

#pragma once

#include <SushiProfiler/clock/calibration.hpp>
#include <SushiProfiler/clock/trace_clock.hpp>
#include <SushiProfiler/collector/drain_batch.hpp>
#include <SushiProfiler/collector/collector_settings.hpp>
#include <SushiProfiler/rings/thread_ring.hpp>
#include <SushiProfiler/sites/name_table.hpp>
#include <SushiProfiler/sites/site_registry.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace SushiProfiler
{
    /**
     * @brief Holds the process's one gate, its sites, its names and every thread's ring.
     *
     * It is the only process-wide state of SushiProfiler, because a recording macro at any call
     * site must reach it. It is built on first use and never destroyed, so a thread that exits
     * after `main` returns still finds it. One consumer calls `drain`.
     */
    class Collector final
    {
        public:
            /// @brief Thread index of a thread with no ring.
            static constexpr std::uint32_t K_NO_THREAD = ~std::uint32_t{0U};

            /// @brief Returns the collector, building it on first use.
            static Collector& instance();

            Collector(const Collector&) = delete;
            Collector& operator=(const Collector&) = delete;
            Collector(Collector&&) = delete;
            Collector& operator=(Collector&&) = delete;

            /// @brief Returns whether the gate is open; one relaxed load.
            [[nodiscard]] static bool recording() noexcept
            {
                return gate_.load(std::memory_order_relaxed) != 0U;
            }

            /// @brief Returns the time events are stamped with: TSC ticks if invariant, else ns.
            [[nodiscard]] static std::uint64_t now() noexcept
            {
                return use_tsc_.load(std::memory_order_relaxed) ? TraceClock::now()
                                                                : TraceClock::steady_now_ns();
            }

            /**
             * @brief Opens the gate with @p settings; false when already open.
             *
             * Events left in the rings from before are discarded, retired rings are freed and
             * the first calibration point is taken.
             */
            bool open(const CollectorSettings& settings);

            /// @brief Closes the gate; the rings keep their events for one more drain.
            void close();

            /// @brief Returns whether the collector is open.
            [[nodiscard]] bool is_open() const noexcept;

            /// @brief Moves every ring's events into @p out and frees the rings of exited threads.
            void drain(DrainBatch& out);

            /// @brief Applies @p filter to every site.
            void apply(const SiteFilter& filter);

            /// @brief Returns the site registry.
            [[nodiscard]] SiteRegistry& sites() noexcept;

            /// @brief Returns the name table.
            [[nodiscard]] NameTable& names() noexcept;

            /// @brief Returns the calibration point `open` took.
            [[nodiscard]] CalibrationPoint first_point() const noexcept;

            /**
             * @brief Returns the calling thread's ring, registering it on first use while open.
             * @return Null while closed for an unregistered thread, after the thread retired, or
             *         when the ring or its lock cannot be had.
             */
            [[nodiscard]] static ThreadRing* ring() noexcept;

            /**
             * @brief Registers the calling thread now, open or closed, so its first record
             *        allocates nothing.
             * @param capacity_events The ring's size; 0 takes the settings' size.
             */
            static bool register_this_thread(std::size_t capacity_events = 0U);

            /// @brief Names the calling thread, registering it first.
            static void set_thread_name(std::string_view name);

            /// @brief Returns the calling thread's index, or K_NO_THREAD without a ring.
            [[nodiscard]] static std::uint32_t thread_index() noexcept;

            /// @brief Retires the calling thread's ring; its exit guards call this.
            static void retire_current_thread() noexcept;

        private:
            /// @brief One thread's ring and what the collector knows of it.
            struct Registration
            {
                std::unique_ptr<ThreadRing> ring;  /**< The thread's ring. */
                std::uint32_t thread_index = 0U;   /**< Its index. */
                bool retired = false;              /**< Whether the thread has exited. */
            };

            Collector();
            ~Collector() = default;

            /// @brief Registers the calling thread with a ring of @p capacity_events.
            ThreadRing* register_current_thread(std::size_t capacity_events);

            /// @brief Returns a calibration point on the clock events are stamped with.
            [[nodiscard]] CalibrationPoint sample() const noexcept;

            /// @brief Whether events are stamped with the TSC; set once by the constructor.
            static std::atomic<bool> use_tsc_;

            /// @brief Non-zero while the collector records.
            static std::atomic<std::uint32_t> gate_;

            /// @brief The sites of the process.
            SiteRegistry sites_;

            /// @brief The interned names of the process.
            NameTable names_;

            /// @brief Guards registrations_, settings_, next_index_ and first_point_.
            mutable std::mutex mutex_;

            /// @brief Every thread with a ring not yet freed.
            std::vector<Registration> registrations_;

            /// @brief The settings of the last open.
            CollectorSettings settings_;

            /// @brief The index the next registered thread gets.
            std::uint32_t next_index_ = 0U;

            /// @brief The calibration point of the last open.
            CalibrationPoint first_point_;

            /// @brief Scratch the drain moves events through before discarding them.
            std::vector<TraceEvent> discard_;
    };
} // namespace SushiProfiler
