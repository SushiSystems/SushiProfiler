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

    Consumer::~Consumer()
    {
        stop();
    }

    void Consumer::start()
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (thread_.joinable())
            return;
        stopping_ = false;
        thread_ = std::thread(&Consumer::run, this);
    }

    void Consumer::stop()
    {
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            if (!thread_.joinable())
                return;
            stopping_ = true;
        }
        wake_.notify_all();
        thread_.join();
    }

    void Consumer::run()
    {
        std::chrono::milliseconds wait = K_FASTEST_POLL;
        std::unique_lock<std::mutex> lock(mutex_);
        const auto stopping_or_woken = [this]
        {
            return stopping_ || woken_;
        };
        while (true)
        {
            wake_.wait_for(lock, wait, stopping_or_woken);
            if (stopping_)
                break;
            woken_ = false;
            lock.unlock();
            const double fullest = drain_once();
            lock.lock();
            wait = next_poll(wait, fullest, slowest_poll());
        }
        lock.unlock();
        static_cast<void>(drain_once());
        assembler_.finish();
    }

    std::uint64_t Consumer::drains() const noexcept
    {
        return drains_.load(std::memory_order_relaxed);
    }

    void Consumer::wake()
    {
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            woken_ = true;
        }
        wake_.notify_all();
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
