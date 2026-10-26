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
 * @file collector.cpp
 * @brief Opens and closes the gate, registers and retires thread rings and drains them.
 * @author Mustafa Garip
 * @see modules/capture/collector/README.md
 */

#include <SushiProfiler/collector/collector.hpp>

#include <new>
#include <system_error>
#include <utility>

namespace SushiProfiler
{
    std::atomic<bool> Collector::use_tsc_{false};
    std::atomic<std::uint32_t> Collector::gate_{0U};

    namespace
    {
        /// @brief The calling thread's ring once registered.
        thread_local ThreadRing* t_ring = nullptr;

        /// @brief The calling thread's index once registered.
        thread_local std::uint32_t t_index = Collector::K_NO_THREAD;

        /// @brief Whether the calling thread's registration was torn down at exit.
        thread_local bool t_retired = false;

        /// @brief Retires the calling thread's ring when the thread exits.
        struct ThreadRetirementGuard
        {
            /// @brief Hands the ring back to the collector for its last drain.
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

    Collector& Collector::instance()
    {
        // Never destroyed: a thread that exits after main returns still retires into it.
        static Collector* const collector = new Collector();
        return *collector;
    }

    Collector::Collector()
    {
        use_tsc_.store(TraceClock::ticks_are_invariant(), std::memory_order_relaxed);
    }

    CalibrationPoint Collector::sample() const noexcept
    {
        if (use_tsc_.load(std::memory_order_relaxed))
            return TraceClock::sample();
        const std::uint64_t steady = TraceClock::steady_now_ns();
        return CalibrationPoint{steady, steady};
    }

    bool Collector::open(const CollectorSettings& settings)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (gate_.load(std::memory_order_relaxed) != 0U)
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
        first_point_ = sample();
        gate_.store(1U, std::memory_order_release);
        return true;
    }

    void Collector::close()
    {
        gate_.store(0U, std::memory_order_release);
    }

    bool Collector::is_open() const noexcept
    {
        return gate_.load(std::memory_order_acquire) != 0U;
    }

    void Collector::drain(DrainBatch& out)
    {
        out.threads.clear();
        out.ticks_are_tsc = use_tsc_.load(std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(mutex_);
        out.point = sample();
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

    ThreadRing* Collector::register_current_thread(std::size_t capacity_events)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        Registration registration;
        registration.ring = std::make_unique<ThreadRing>(
            capacity_events != 0U ? capacity_events : settings_.ring_events);
        registration.thread_index = next_index_++;
        ThreadRing* const ring = registration.ring.get();
        registrations_.push_back(std::move(registration));
        arm_thread_retirement();
        t_index = registrations_.back().thread_index;
        t_ring = ring;
        return ring;
    }

    ThreadRing* Collector::ring() noexcept
    {
        ThreadRing* const ring = t_ring;
        if (ring != nullptr)
            return ring;
        if (t_retired || !recording())
            return nullptr;
        try
        {
            return instance().register_current_thread(0U);
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

    bool Collector::register_this_thread(std::size_t capacity_events)
    {
        if (t_ring != nullptr)
            return true;
        if (t_retired)
            return false;
        return instance().register_current_thread(capacity_events) != nullptr;
    }

    void Collector::set_thread_name(std::string_view name)
    {
        if (register_this_thread())
            t_ring->set_name(name);
    }

    std::uint32_t Collector::thread_index() noexcept
    {
        return t_index;
    }

    void Collector::retire_current_thread() noexcept
    {
        ThreadRing* const ring = t_ring;
        t_ring = nullptr;
        t_retired = true;
        if (ring == nullptr)
            return;
        Collector& collector = instance();
        std::lock_guard<std::mutex> lock(collector.mutex_);
        for (Registration& registration : collector.registrations_)
        {
            if (registration.ring.get() == ring)
                registration.retired = true;
        }
    }
} // namespace SushiProfiler
