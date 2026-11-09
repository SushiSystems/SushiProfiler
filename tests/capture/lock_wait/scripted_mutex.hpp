/**************************************************************************/
/* scripted_mutex.hpp                                                     */
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
 * @file scripted_mutex.hpp
 * @brief Declares a Lockable that another thread seems to hold, with no thread behind it.
 * @author Mustafa Garip
 * @see tests/capture/lock_wait/README.md
 */

#pragma once

#include <SushiProfiler/clock/manual_tick_source.hpp>

#include <cstdint>

namespace SushiProfilerTests
{
    /**
     * @brief A mutex whose contention is scripted: `try_lock` fails while another holder is
     *        pretended, and `lock` ends that hold after a wait of a set length.
     *
     * The wait moves a manual clock, so a test reads the length of a wait it set itself.
     */
    class ScriptedMutex final
    {
        public:
            /**
             * @brief Builds a mutex that is held by another thread when @p held_by_other is true.
             * @param clock The clock `lock` advances by @p wait_ns, or null to leave time alone.
             * @param wait_ns How long the pretended holder keeps the mutex once `lock` starts.
             */
            explicit ScriptedMutex(bool held_by_other,
                                   SushiProfiler::ManualTickSource* clock = nullptr,
                                   std::uint64_t wait_ns = 0U) noexcept
                : held_by_other_(held_by_other), clock_(clock), wait_ns_(wait_ns)
            {
            }

            /// @brief Takes the mutex if neither the pretended holder nor the caller holds it.
            bool try_lock() noexcept
            {
                if (held_by_other_ || locked_)
                    return false;
                locked_ = true;
                return true;
            }

            /// @brief Waits out the pretended holder, then takes the mutex.
            void lock() noexcept
            {
                if (held_by_other_ && clock_ != nullptr)
                    clock_->advance(wait_ns_);
                held_by_other_ = false;
                locked_ = true;
            }

            /// @brief Releases the mutex.
            void unlock() noexcept
            {
                locked_ = false;
            }

            /// @brief Returns whether the caller holds the mutex.
            [[nodiscard]] bool locked() const noexcept
            {
                return locked_;
            }

        private:
            /// @brief Whether a pretended holder still owns the mutex.
            bool held_by_other_;

            /// @brief The clock `lock` advances, or null.
            SushiProfiler::ManualTickSource* clock_;

            /// @brief The length of the wait in nanoseconds.
            std::uint64_t wait_ns_;

            /// @brief Whether the caller holds the mutex.
            bool locked_ = false;
    };
} // namespace SushiProfilerTests
