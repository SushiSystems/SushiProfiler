/**************************************************************************/
/* manual_poll_clock.cpp                                                  */
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
 * @file manual_poll_clock.cpp
 * @brief Keeps the time and the waits of the in-memory poll clock.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#include <SushiProfiler/consumer/manual_poll_clock.hpp>

namespace SushiProfiler
{
    bool ManualPollClock::wait(std::chrono::milliseconds limit)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        limits_.push_back(limit);
        waiting_ = true;
        const std::chrono::milliseconds deadline = elapsed_ + limit;
        entered_.notify_all();
        changed_.wait(lock,
                      [this, deadline] { return stopping_ || woken_ || elapsed_ >= deadline; });
        waiting_ = false;
        if (stopping_)
            return false;
        woken_ = false;
        return true;
    }

    void ManualPollClock::wake()
    {
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            woken_ = true;
        }
        changed_.notify_all();
    }

    void ManualPollClock::stop()
    {
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        changed_.notify_all();
    }

    void ManualPollClock::rearm()
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = false;
    }

    void ManualPollClock::advance(std::chrono::milliseconds elapsed)
    {
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            elapsed_ += elapsed;
        }
        changed_.notify_all();
    }

    void ManualPollClock::poll_once()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        entered_.wait(lock, [this] { return waiting_; });
        const std::size_t waits = limits_.size();
        elapsed_ += limits_.back();
        changed_.notify_all();
        entered_.wait(lock, [this, waits] { return limits_.size() > waits && waiting_; });
    }

    void ManualPollClock::await_waits(std::size_t count)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        entered_.wait(lock, [this, count] { return limits_.size() >= count; });
    }

    std::vector<std::chrono::milliseconds> ManualPollClock::limits() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return limits_;
    }
} // namespace SushiProfiler
