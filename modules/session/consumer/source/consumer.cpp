/**************************************************************************/
/* consumer.cpp                                                           */
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
 * @file consumer.cpp
 * @brief Waits between drains for as long as the rings' fill allows, then drains and assembles.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#include <SushiProfiler/consumer/consumer.hpp>

#include <algorithm>

namespace SushiProfiler
{
    std::chrono::milliseconds next_poll(std::chrono::milliseconds current, double fullest) noexcept
    {
        return next_poll(current, fullest, K_SLOWEST_POLL);
    }

    std::chrono::milliseconds next_poll(std::chrono::milliseconds current, double fullest,
                                        std::chrono::milliseconds slowest) noexcept
    {
        if (fullest > 0.5)
            return K_FASTEST_POLL;
        if (fullest >= 0.25)
            return std::clamp(current, K_FASTEST_POLL, slowest);
        return std::min(current * 2, slowest);
    }

    Consumer::Consumer(Collector& collector, PeriodAssembler& assembler) noexcept
        : collector_(collector), assembler_(assembler)
    {

    }

    Consumer::Consumer(Collector& collector, PeriodAssembler& assembler, const LiveChannel& live,
                       std::chrono::milliseconds live_slowest) noexcept
        : collector_(collector), assembler_(assembler), live_(&live), live_slowest_(live_slowest)
    {

    }

    Consumer::Consumer(Collector& collector, PeriodAssembler& assembler, const LiveChannel& live,
                       std::chrono::milliseconds live_slowest, PollClock& clock) noexcept
        : collector_(collector), assembler_(assembler), live_(&live), live_slowest_(live_slowest),
          clock_(&clock)
    {

    }

    Consumer::~Consumer()
    {
        stop();
    }

    void Consumer::start()
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (thread_.joinable())
            return;
        clock_->rearm();
        thread_ = std::thread(&Consumer::run, this);
    }

    void Consumer::stop()
    {
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            if (!thread_.joinable())
                return;
            clock_->stop();
        }
        thread_.join();
    }

    void Consumer::run()
    {
        std::chrono::milliseconds wait = K_FASTEST_POLL;
        while (clock_->wait(wait))
        {
            const double fullest = drain_once();
            wait = next_poll(wait, fullest, slowest_poll());
        }
        static_cast<void>(drain_once());
        assembler_.finish();
    }

    std::uint64_t Consumer::drains() const noexcept
    {
        return drains_.load(std::memory_order_relaxed);
    }

    void Consumer::wake()
    {
        clock_->wake();
    }

    std::chrono::milliseconds Consumer::slowest_poll() const noexcept
    {
        if (live_ != nullptr && live_->watched())
            return std::min(live_slowest_, K_SLOWEST_POLL);
        return K_SLOWEST_POLL;
    }

    double Consumer::drain_once()
    {
        drains_.fetch_add(1U, std::memory_order_relaxed);
        collector_.drain(batch_);
        assembler_.add(batch_);
        double fullest = 0.0;
        for (const ThreadDrain& thread : batch_.threads)
            fullest = std::max(fullest, thread.fill);
        return fullest;
    }
} // namespace SushiProfiler
