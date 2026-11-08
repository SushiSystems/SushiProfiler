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
 * @file collector.hpp
 * @brief Declares the collector: a gate, its thread rings and the drain of every record.
 * @author Mustafa Garip
 * @see modules/capture/collector/README.md
 */

#pragma once

#include <SushiProfiler/clock/calibration.hpp>
#include <SushiProfiler/clock/tick_source.hpp>
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
#include <thread>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Holds one gate, its sites, its names and every thread's ring for that gate.
     *
     * A caller that owns a collector passes it to what records into it. The process-wide one,
     * instance(), is what the recording macros reach; it is built on first use and never
     * destroyed, so a thread that exits after `main` returns still finds it. One consumer
     * calls `drain`.
     */
    class SUSHIPROFILER_API Collector final
    {
        public:
            /// @brief Thread index of a thread with no ring.
            static constexpr std::uint32_t K_NO_THREAD = ~std::uint32_t{0U};

            /**
             * @brief Builds a closed collector that stamps events with @p clock.
             *
             * Its site registry keeps the ids it gives, so the site handles the process-wide
             * collector caches its ids in stay untouched.
             * @pre @p clock outlives the collector.
             */
            explicit Collector(const TickSource& clock);

            Collector(const Collector&) = delete;
            Collector& operator=(const Collector&) = delete;
            Collector(Collector&&) = delete;
            Collector& operator=(Collector&&) = delete;

            /// @brief Frees every ring; a thread that kept one of them records into it no more.
            ~Collector();

            /// @brief Returns the process-wide collector, building it on first use.
            static Collector& instance();

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

            /// @brief Returns whether a session is open, paused or not.
            [[nodiscard]] bool is_open() const noexcept;

            /// @brief Returns whether the gate is open; one relaxed load.
            [[nodiscard]] bool is_recording() const noexcept
            {
                return gate_.load(std::memory_order_relaxed) != 0U;
            }

            /// @brief Returns whether period marks are written; one relaxed load.
            [[nodiscard]] bool is_marking() const noexcept
            {
                return marking_.load(std::memory_order_relaxed) != 0U;
            }

            /// @brief Returns the stamp an event is given now: what the tick source reads.
            [[nodiscard]] std::uint64_t stamp() const noexcept
            {
                return clock_.now();
            }

            /// @brief Returns whether a session wants its sources to measure; one relaxed load.
            [[nodiscard]] bool wants_sources() const noexcept
            {
                return sources_wanted_.load(std::memory_order_relaxed);
            }

            /// @brief Sets what wants_sources() returns; the session calls it around its sources.
            void set_sources_wanted(bool wanted) noexcept;

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
            [[nodiscard]] ThreadRing* thread_ring() noexcept;

            /**
             * @brief Returns the calling thread's ring for a period mark, registering it on first
             *        use while marks are written, even when the gate is held.
             */
            [[nodiscard]] ThreadRing* thread_mark_ring() noexcept;

            /**
             * @brief Registers the calling thread now, open or closed, so its first record
             *        allocates nothing.
             * @param capacity_events The ring's size; 0 takes the settings' size.
             */
            bool register_thread(std::size_t capacity_events = 0U);

            /// @brief Names the calling thread's ring, registering it first.
            void name_thread(std::string_view name);

            /// @brief Returns the calling thread's index, or K_NO_THREAD without a ring.
            [[nodiscard]] std::uint32_t index_of_thread() const noexcept;

            /// @brief Returns whether the process-wide collector's gate is open; two loads.
            [[nodiscard]] static bool recording() noexcept
            {
                const Collector* const collector = process_wide_.load(std::memory_order_acquire);
                return collector != nullptr && collector->is_recording();
            }

            /// @brief Returns the process-wide collector's stamp of now.
            [[nodiscard]] static std::uint64_t now() noexcept;

            /// @brief Returns whether the process-wide collector writes period marks.
            [[nodiscard]] static bool marking() noexcept
            {
                const Collector* const collector = process_wide_.load(std::memory_order_acquire);
                return collector != nullptr && collector->is_marking();
            }

            /// @brief Returns whether the process-wide collector's session wants its sources.
            [[nodiscard]] static bool sources_wanted() noexcept
            {
                const Collector* const collector = process_wide_.load(std::memory_order_acquire);
                return collector != nullptr && collector->wants_sources();
            }

            /// @brief Sets what sources_wanted() returns, on the process-wide collector.
            static void want_sources(bool wanted) noexcept;

            /// @brief Returns thread_ring() of the process-wide collector.
            [[nodiscard]] static ThreadRing* ring() noexcept;

            /// @brief Returns thread_mark_ring() of the process-wide collector.
            [[nodiscard]] static ThreadRing* mark_ring() noexcept;

            /// @brief Returns register_thread() of the process-wide collector.
            static bool register_this_thread(std::size_t capacity_events = 0U);

            /// @brief Runs name_thread() on the process-wide collector.
            static void set_thread_name(std::string_view name);

            /// @brief Returns index_of_thread() of the process-wide collector.
            [[nodiscard]] static std::uint32_t thread_index() noexcept;

            /// @brief Retires the calling thread's rings in every collector, as its exit guards do.
            static void retire_current_thread() noexcept;

        private:
            /// @brief Builds a closed collector on @p clock that keeps site ids where @p cache says.
            Collector(const TickSource& clock, SiteCache cache);

            /// @brief One thread's ring and what the collector knows of it.
            struct Registration
            {
                std::unique_ptr<ThreadRing> ring;  /**< The thread's ring. */
                std::thread::id thread;            /**< The thread it belongs to. */
                std::uint32_t thread_index = 0U;   /**< Its index. */
                bool retired = false;              /**< Whether the thread has exited. */
            };

            /**
             * @brief Returns the calling thread's ring, looked up under the lock, or a new one of
             *        @p capacity_events when it has none and @p may_register holds.
             */
            ThreadRing* attach_current_thread(std::size_t capacity_events, bool may_register);

            /// @brief Returns attach_current_thread(0, @p may_register), or null on a failure.
            [[nodiscard]] ThreadRing* attach_or_null(bool may_register) noexcept;

            /// @brief Marks the rings of @p thread retired.
            void retire_thread(std::thread::id thread) noexcept;

            /// @brief Sets the gate from marking_ and held_; the caller holds mutex_.
            void apply_gate_locked() noexcept;

            /// @brief The process-wide collector once instance() built it, else null.
            static std::atomic<Collector*> process_wide_;

            /// @brief The tick source events are stamped with.
            const TickSource& clock_;

            /// @brief A number no other collector of the process carries; thread caches key on it.
            const std::uint64_t serial_;

            /// @brief Non-zero while the collector records zones: marking and not held.
            std::atomic<std::uint32_t> gate_{0U};

            /// @brief Non-zero while period marks are written: open and not paused by the window.
            std::atomic<std::uint32_t> marking_{0U};

            /// @brief Whether a session wants its sources to measure.
            std::atomic<bool> sources_wanted_{false};

            /// @brief Whether hold() is in effect.
            std::atomic<bool> held_{false};

            /// @brief Whether a session is open, between `open` and `close`.
            std::atomic<bool> open_{false};

            /// @brief The sites of the collector.
            SiteRegistry sites_;

            /// @brief The interned names of the collector.
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
