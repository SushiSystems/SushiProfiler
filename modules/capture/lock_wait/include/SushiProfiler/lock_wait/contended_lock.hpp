/**************************************************************************/
/* contended_lock.hpp                                                     */
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
 * @file contended_lock.hpp
 * @brief Declares the lock guard that records the time a contended lock waits, and its macros.
 * @author Mustafa Garip
 * @see modules/capture/lock_wait/README.md
 */

#pragma once

#include <SushiProfiler/sites/site.hpp>
#include <SushiProfiler/zones/macros.hpp>
#include <SushiProfiler/zones/zone.hpp>

#include <type_traits>

namespace SushiProfiler
{
    /**
     * @brief Holds @p Mutex like `std::unique_lock`, and records a zone only when it had to wait.
     * @tparam Mutex A Lockable type: `try_lock`, `lock` and `unlock`.
     * @tparam Compiled False compiles the wait zone out and leaves a plain lock.
     *
     * An uncontended lock costs one `try_lock` and writes nothing. The guard is BasicLockable,
     * so `std::condition_variable_any` can wait on it.
     */
    template <class Mutex, bool Compiled = true>
    class ContendedLock final
    {
        public:
            /// @brief Locks @p mutex, recording the wait at @p handle's site if it is contended.
            ContendedLock(Mutex& mutex, SiteSlot<Compiled>& handle) : mutex_(mutex), handle_(handle)
            {
                lock();
            }

            ContendedLock(const ContendedLock&) = delete;
            ContendedLock& operator=(const ContendedLock&) = delete;
            ContendedLock(ContendedLock&&) = delete;
            ContendedLock& operator=(ContendedLock&&) = delete;

            /// @brief Unlocks the mutex if the guard holds it.
            ~ContendedLock()
            {
                if (owns_)
                    mutex_.unlock();
            }

            /// @brief Locks the mutex again, recording the wait as the constructor does.
            /// @pre The guard does not hold the mutex.
            void lock()
            {
                if (!mutex_.try_lock())
                {
                    [[maybe_unused]] const ScopedZone<Compiled> wait(handle_);
                    mutex_.lock();
                }
                owns_ = true;
            }

            /// @brief Unlocks the mutex.
            /// @pre The guard holds the mutex.
            void unlock()
            {
                owns_ = false;
                mutex_.unlock();
            }

            /// @brief Returns whether the guard holds the mutex.
            [[nodiscard]] bool owns_lock() const noexcept
            {
                return owns_;
            }

        private:
            /// @brief The guarded mutex.
            Mutex& mutex_;

            /// @brief The site the waits are recorded at.
            SiteSlot<Compiled>& handle_;

            /// @brief Whether the guard holds the mutex.
            bool owns_ = false;
    };
} // namespace SushiProfiler

/// @brief Declares lock wait site @p id and locks @p mutex for the rest of the scope.
#define SP_DETAIL_LOCK_WAIT(id, compiled, name, mutex)                                            \
    SP_DETAIL_SITE(id, ((compiled) >= 1), name, "Lock wait", 1, LockWait, Add, nullptr);           \
    const ::SushiProfiler::ContendedLock<std::remove_reference_t<decltype(mutex)>,                 \
                                         ((compiled) >= 1)>                                        \
        SP_DETAIL_JOIN(sp_lock_, id)(mutex, SP_DETAIL_JOIN(sp_handle_, id))

/// @brief Locks @p mutex for the rest of the scope, recording its wait when @p compiled is on.
#define SP_LOCK_WAIT_AT(compiled, name, mutex)                                                    \
    SP_DETAIL_LOCK_WAIT(__COUNTER__, compiled, name, mutex)

/// @brief Locks @p mutex for the rest of the scope, recording its wait.
#define SP_LOCK_WAIT(name, mutex) SP_LOCK_WAIT_AT(SP_PROFILE_LEVEL, name, mutex)
