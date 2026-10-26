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
#include <SushiProfiler/version/export.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Holds the process's one gate, its sites, its names and every thread's ring.
     *
     * It is the only process-wide state of SushiProfiler, because a recording macro at any call
     * site must reach it. It is built on first use and never destroyed, so a thread that exits
     * after `main` returns still finds it. One consumer calls `drain`.
     */
    class SUSHIPROFILER_API Collector final
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

            /// @brief Ends the session and closes the gate; the rings keep their events to drain.
            void close();

            /// @brief Closes the gate and period marks inside an open session, keeping the rings.
            void pause();

            /// @brief Opens marks again, and the gate unless held; does nothing while closed.
            void resume();

            /**
             * @brief Closes the zone gate inside an open session until release(), the way a
             *        caller pauses recording; period marks are still written.
             *
             * It is a second reason to close the gate beside pause(), so the window's next
             * resume() cannot undo it. Does nothing while the collector is closed.
             */
            void hold();

            /// @brief Ends hold(); the gate opens again where the window records.
            void release();

            /// @brief Returns whether hold() is in effect.
            [[nodiscard]] bool is_held() const noexcept;

            /// @brief Returns whether period marks are written; one relaxed load.
            [[nodiscard]] static bool marking() noexcept
            {
                return marking_.load(std::memory_order_relaxed) != 0U;
            }

            /// @brief Returns whether a session wants its sources to measure; one relaxed load.
            [[nodiscard]] static bool sources_wanted() noexcept
            {
                return sources_wanted_.load(std::memory_order_relaxed);
            }

            /// @brief Sets what sources_wanted() returns; the session calls it around its sources.
            static void want_sources(bool wanted) noexcept;

            /// @brief Returns whether a session is open, paused or not.
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
             * @brief Returns the calling thread's ring for a period mark, registering it on first
             *        use while marks are written, even when the gate is held.
             */
            [[nodiscard]] static ThreadRing* mark_ring() noexcept;

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

            /// @brief Non-zero while the collector records zones: marking and not held.
            static std::atomic<std::uint32_t> gate_;

            /// @brief Non-zero while period marks are written: open and not paused by the window.
            static std::atomic<std::uint32_t> marking_;

            /// @brief Whether a session wants its sources to measure.
            static std::atomic<bool> sources_wanted_;

            /// @brief Whether hold() is in effect.
            std::atomic<bool> held_{false};

            /// @brief Sets the gate from marking_ and held_; the caller holds mutex_.
            void apply_gate_locked() noexcept;

            /// @brief Whether a session is open, between `open` and `close`.
            std::atomic<bool> open_{false};

            /// @brief The sites of the process.
            SiteRegistry sites_;

            /// @brief The interned names of the process.
            NameTable names_;

            /// @brief Guards the registrations, the settings, the first point and the gate's moves.
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
SUSHIPROFILER_DLL_WARNINGS_POP
