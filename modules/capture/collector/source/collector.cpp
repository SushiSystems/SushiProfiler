/**************************************************************************/
/* collector.cpp                                                          */
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
 * @file collector.cpp
 * @brief Opens and closes the gate, registers and retires thread rings and drains them.
 * @author Mustafa Garip
 * @see modules/capture/collector/README.md
 */

#include <SushiProfiler/collector/collector.hpp>

#include <algorithm>
#include <new>
#include <system_error>
#include <utility>

#include <SushiProfiler/clock/trace_tick_source.hpp>

namespace SushiProfiler
{
    std::atomic<Collector*> Collector::process_wide_{nullptr};

    namespace
    {
        /// @brief The serial the next collector takes; 0 is never taken.
        std::atomic<std::uint64_t> g_next_serial{1U};

        /// @brief The collector a thread last reached and the ring and index it has there.
        struct ThreadSlot
        {
            std::uint64_t serial = 0U;                        /**< The collector's serial, or 0. */
            ThreadRing* ring = nullptr;                       /**< The thread's ring in it. */
            std::uint32_t index = Collector::K_NO_THREAD;     /**< The thread's index in it. */
        };

        /// @brief The calling thread's cache of its last collector.
        thread_local ThreadSlot t_slot;

        /// @brief Whether the calling thread's registrations were torn down at exit.
        thread_local bool t_retired = false;

        /// @brief Every collector alive in the process, which a retiring thread visits.
        struct LiveCollectors
        {
            std::mutex mutex;                    /**< Guards collectors. */
            std::vector<Collector*> collectors;  /**< The collectors. */
        };

        /// @brief Returns the list of live collectors; never destroyed, like the process-wide one.
        LiveCollectors& live_collectors()
        {
            static LiveCollectors* const live = new LiveCollectors();
            return *live;
        }

        /// @brief Retires the calling thread's rings when the thread exits.
        struct ThreadRetirementGuard
        {
            /// @brief Hands the rings back to their collectors for a last drain.
            ~ThreadRetirementGuard()
            {
                Collector::retire_current_thread();
            }
        };

        /// @brief Retires the thread last where a teardown-built guard never runs; see README.
        thread_local ThreadRetirementGuard t_exit_backstop;

        /// @brief Builds the thread's two retirement guards; the backstop is built first.
        void arm_thread_retirement()
        {
            static_cast<void>(&t_exit_backstop);
            thread_local ThreadRetirementGuard registration;
            static_cast<void>(&registration);
        }
    } // namespace

    Collector::Collector(const TickSource& clock) : Collector(clock, SiteCache::InRegistry)
    {

    }

    Collector::Collector(const TickSource& clock, SiteCache cache)
        : clock_(clock), serial_(g_next_serial.fetch_add(1U, std::memory_order_relaxed)),
          sites_(SiteRegistry::K_DEFAULT_SITE_CAPACITY, cache)
    {
        LiveCollectors& live = live_collectors();
        const std::lock_guard<std::mutex> lock(live.mutex);
        live.collectors.push_back(this);
    }

    Collector::~Collector()
    {
        LiveCollectors& live = live_collectors();
        const std::lock_guard<std::mutex> lock(live.mutex);
        live.collectors.erase(std::remove(live.collectors.begin(), live.collectors.end(), this),
                              live.collectors.end());
    }

    Collector& Collector::instance()
    {
        // Never destroyed: a thread that exits after main returns still retires into it.
        static Collector* const collector = []
        {
            static const TraceTickSource* const clock = new TraceTickSource();
            Collector* const built = new Collector(*clock, SiteCache::InHandle);
            process_wide_.store(built, std::memory_order_release);
            return built;
        }();
        return *collector;
    }

    bool Collector::open(const CollectorSettings& settings)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (open_.load(std::memory_order_relaxed))
            return false;
        settings_ = settings;
        sites_.apply(settings.filter);
        std::vector<Registration> kept;
        kept.reserve(registrations_.size());
        for (Registration& registration : registrations_)
        {
            discard_.clear();
            registration.ring->drain(discard_);
            static_cast<void>(registration.ring->take_dropped());
            if (!registration.retired)
                kept.push_back(std::move(registration));
        }
        registrations_ = std::move(kept);
        discard_.clear();
        first_point_ = clock_.sample();
        held_.store(false, std::memory_order_relaxed);
        open_.store(true, std::memory_order_release);
        marking_.store(1U, std::memory_order_release);
        apply_gate_locked();
        return true;
    }

    void Collector::close()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        open_.store(false, std::memory_order_release);
        held_.store(false, std::memory_order_relaxed);
        marking_.store(0U, std::memory_order_release);
        gate_.store(0U, std::memory_order_release);
    }

    void Collector::pause()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        marking_.store(0U, std::memory_order_release);
        gate_.store(0U, std::memory_order_release);
    }

    void Collector::resume()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!open_.load(std::memory_order_relaxed))
            return;
        marking_.store(1U, std::memory_order_release);
        apply_gate_locked();
    }

    void Collector::hold()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!open_.load(std::memory_order_relaxed))
            return;
        held_.store(true, std::memory_order_relaxed);
        apply_gate_locked();
    }

    void Collector::release()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        held_.store(false, std::memory_order_relaxed);
        if (open_.load(std::memory_order_relaxed))
            apply_gate_locked();
    }

    bool Collector::is_held() const noexcept
    {
        return held_.load(std::memory_order_relaxed);
    }

    bool Collector::is_open() const noexcept
    {
        return open_.load(std::memory_order_acquire);
    }

    void Collector::set_sources_wanted(bool wanted) noexcept
    {
        sources_wanted_.store(wanted, std::memory_order_relaxed);
    }

    void Collector::apply_gate_locked() noexcept
    {
        const bool open = marking_.load(std::memory_order_relaxed) != 0U &&
                          !held_.load(std::memory_order_relaxed);
        gate_.store(open ? 1U : 0U, std::memory_order_release);
    }

    void Collector::drain(DrainBatch& out)
    {
        out.threads.clear();
        out.ticks_are_tsc = clock_.ticks_are_tsc();
        std::lock_guard<std::mutex> lock(mutex_);
        out.point = clock_.sample();
        out.threads.reserve(registrations_.size());
        std::vector<Registration> kept;
        kept.reserve(registrations_.size());
        for (Registration& registration : registrations_)
        {
            ThreadDrain thread;
            thread.thread_index = registration.thread_index;
            thread.name = registration.ring->name();
            thread.fill = registration.ring->fill();
            registration.ring->drain(thread.events);
            thread.dropped = registration.ring->take_dropped();
            thread.retired = registration.retired;
            out.threads.push_back(std::move(thread));
            if (!registration.retired)
                kept.push_back(std::move(registration));
        }
        registrations_ = std::move(kept);
    }

    void Collector::apply(const SiteFilter& filter)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        settings_.filter = filter;
        sites_.apply(filter);
    }

    SiteRegistry& Collector::sites() noexcept
    {
        return sites_;
    }

    NameTable& Collector::names() noexcept
    {
        return names_;
    }

    CalibrationPoint Collector::first_point() const noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return first_point_;
    }

    ThreadRing* Collector::attach_current_thread(std::size_t capacity_events, bool may_register)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const std::thread::id self = std::this_thread::get_id();
        for (const Registration& registration : registrations_)
        {
            if (registration.thread == self && !registration.retired)
            {
                t_slot = ThreadSlot{serial_, registration.ring.get(), registration.thread_index};
                return registration.ring.get();
            }
        }
        if (!may_register)
            return nullptr;
        Registration registration;
        registration.ring = std::make_unique<ThreadRing>(
            capacity_events != 0U ? capacity_events : settings_.ring_events);
        registration.thread = self;
        registration.thread_index = next_index_++;
        ThreadRing* const ring = registration.ring.get();
        const std::uint32_t index = registration.thread_index;
        registrations_.push_back(std::move(registration));
        arm_thread_retirement();
        t_slot = ThreadSlot{serial_, ring, index};
        return ring;
    }

    ThreadRing* Collector::attach_or_null(bool may_register) noexcept
    {
        try
        {
            return attach_current_thread(0U, may_register);
        }
        catch (const std::bad_alloc&)
        {
            return nullptr;
        }
        catch (const std::system_error&)
        {
            return nullptr;
        }
    }

    ThreadRing* Collector::thread_ring() noexcept
    {
        if (t_slot.serial == serial_)
            return t_slot.ring;
        // A thread whose slot was never filled has no ring in any collector.
        if (t_retired || (t_slot.serial == 0U && !is_recording()))
            return nullptr;
        return attach_or_null(is_recording());
    }

    ThreadRing* Collector::thread_mark_ring() noexcept
    {
        if (t_slot.serial == serial_)
            return t_slot.ring;
        // A thread whose slot was never filled has no ring in any collector.
        if (t_retired || (t_slot.serial == 0U && !is_marking()))
            return nullptr;
        return attach_or_null(is_marking());
    }

    bool Collector::register_thread(std::size_t capacity_events)
    {
        if (t_slot.serial == serial_)
            return true;
        if (t_retired)
            return false;
        return attach_current_thread(capacity_events, true) != nullptr;
    }

    void Collector::name_thread(std::string_view name)
    {
        if (register_thread())
            t_slot.ring->set_name(name);
    }

    std::uint32_t Collector::index_of_thread() const noexcept
    {
        if (t_slot.serial == serial_)
            return t_slot.index;
        if (t_slot.serial == 0U)
            return K_NO_THREAD;
        const std::thread::id self = std::this_thread::get_id();
        try
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const Registration& registration : registrations_)
            {
                if (registration.thread == self && !registration.retired)
                    return registration.thread_index;
            }
        }
        catch (const std::system_error&)
        {
            return K_NO_THREAD;
        }
        return K_NO_THREAD;
    }

    void Collector::retire_thread(std::thread::id thread) noexcept
    {
        try
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (Registration& registration : registrations_)
            {
                if (registration.thread == thread)
                    registration.retired = true;
            }
        }
        catch (const std::system_error&)
        {
            return;
        }
    }

    std::uint64_t Collector::now() noexcept
    {
        const Collector* const collector = process_wide_.load(std::memory_order_acquire);
        return collector != nullptr ? collector->stamp() : instance().stamp();
    }

    void Collector::want_sources(bool wanted) noexcept
    {
        instance().set_sources_wanted(wanted);
    }

    ThreadRing* Collector::ring() noexcept
    {
        Collector* const collector = process_wide_.load(std::memory_order_acquire);
        return collector != nullptr ? collector->thread_ring() : nullptr;
    }

    ThreadRing* Collector::mark_ring() noexcept
    {
        Collector* const collector = process_wide_.load(std::memory_order_acquire);
        return collector != nullptr ? collector->thread_mark_ring() : nullptr;
    }

    bool Collector::register_this_thread(std::size_t capacity_events)
    {
        return instance().register_thread(capacity_events);
    }

    void Collector::set_thread_name(std::string_view name)
    {
        instance().name_thread(name);
    }

    std::uint32_t Collector::thread_index() noexcept
    {
        const Collector* const collector = process_wide_.load(std::memory_order_acquire);
        return collector != nullptr ? collector->index_of_thread() : K_NO_THREAD;
    }

    void Collector::retire_current_thread() noexcept
    {
        t_slot = ThreadSlot{};
        t_retired = true;
        const std::thread::id self = std::this_thread::get_id();
        try
        {
            LiveCollectors& live = live_collectors();
            const std::lock_guard<std::mutex> lock(live.mutex);
            for (Collector* const collector : live.collectors)
                collector->retire_thread(self);
        }
        catch (const std::bad_alloc&)
        {
            return;
        }
        catch (const std::system_error&)
        {
            return;
        }
    }
} // namespace SushiProfiler
