/**************************************************************************/
/* named_lock_wait.hpp                                                    */
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
 * @file named_lock_wait.hpp
 * @brief Declares the lock-wait macro that declares a std::unique_lock a condition variable takes.
 * @author Mustafa Garip
 * @see modules/capture/lock_wait/README.md
 */

#pragma once

#include <SushiProfiler/sites/site.hpp>
#include <SushiProfiler/zones/macros.hpp>
#include <SushiProfiler/zones/zone.hpp>

#include <mutex>
#include <type_traits>

namespace SushiProfiler
{
    /**
     * @brief Locks @p lock, recording a zone at @p handle's site only when it had to wait.
     * @tparam Compiled False compiles the wait zone out and leaves a plain lock.
     * @pre @p lock holds no mutex.
     *
     * An uncontended lock costs one `try_lock` and writes nothing. It measures this first
     * acquisition only: a later re-lock inside `std::condition_variable::wait` is not timed.
     */
    template <bool Compiled, class Mutex>
    void lock_with_wait(std::unique_lock<Mutex>& lock, SiteSlot<Compiled>& handle)
    {
        if (lock.try_lock())
            return;
        [[maybe_unused]] const ScopedZone<Compiled> wait(handle);
        lock.lock();
    }

    /// @brief Locks @p lock as above, recording the wait at @p handle's site in @p collector.
    template <bool Compiled, class Mutex>
    void lock_with_wait(Collector& collector, std::unique_lock<Mutex>& lock,
                        SiteSlot<Compiled>& handle)
    {
        if (lock.try_lock())
            return;
        [[maybe_unused]] const ScopedZone<Compiled> wait(collector, handle);
        lock.lock();
    }
} // namespace SushiProfiler

/// @brief Declares lock wait site @p id and a locked `std::unique_lock` named @p lock on @p mutex.
#define SP_DETAIL_LOCK_WAIT_NAMED(id, compiled, lock, name, mutex)                                \
    SP_DETAIL_SITE(id, ((compiled) >= 1), name, "Lock wait", 1, LockWait, Add, nullptr);           \
    std::unique_lock<std::remove_reference_t<decltype(mutex)>> lock((mutex), std::defer_lock);     \
    ::SushiProfiler::lock_with_wait<((compiled) >= 1)>(lock, SP_DETAIL_JOIN(sp_handle_, id))

/// @brief Declares `std::unique_lock lock` on @p mutex and locks it, recording its wait when
/// @p compiled is on. The lock is not const, so a condition variable can wait on it.
#define SP_LOCK_WAIT_NAMED_AT(compiled, lock, name, mutex)                                        \
    SP_DETAIL_LOCK_WAIT_NAMED(__COUNTER__, compiled, lock, name, mutex)

/// @brief Declares `std::unique_lock lock` on @p mutex and locks it, recording its wait.
#define SP_LOCK_WAIT_NAMED(lock, name, mutex)                                                     \
    SP_LOCK_WAIT_NAMED_AT(SP_PROFILE_LEVEL, lock, name, mutex)
