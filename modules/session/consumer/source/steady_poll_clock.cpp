/**************************************************************************/
/* steady_poll_clock.cpp                                                  */
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
 * @file steady_poll_clock.cpp
 * @brief Waits on a condition variable for the time a poll asks.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#include <SushiProfiler/consumer/steady_poll_clock.hpp>

namespace SushiProfiler
{
    bool SteadyPollClock::wait(std::chrono::milliseconds limit)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        changed_.wait_for(lock, limit, [this] { return stopping_ || woken_; });
        if (stopping_)
            return false;
        woken_ = false;
        return true;
    }

    void SteadyPollClock::wake()
    {
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            woken_ = true;
        }
        changed_.notify_all();
    }

    void SteadyPollClock::stop()
    {
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        changed_.notify_all();
    }

    void SteadyPollClock::rearm()
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = false;
    }
} // namespace SushiProfiler
