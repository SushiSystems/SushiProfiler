/**************************************************************************/
/* live_channel.cpp                                                       */
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
 * @file live_channel.cpp
 * @brief Keeps the newest closed period and a ring of the last ones, moving pointers only.
 * @author Mustafa Garip
 * @see modules/session/live/README.md
 */

#include <SushiProfiler/live/live_channel.hpp>

#include <algorithm>
#include <utility>

namespace SushiProfiler
{
    LiveChannel::LiveChannel(std::size_t history_capacity)
        : capacity_(std::max<std::size_t>(history_capacity, 1U)), ring_(capacity_)
    {

    }

    std::uint64_t LiveChannel::begin_run()
    {
        std::vector<std::shared_ptr<const ClosedPeriod>> dropped(capacity_);
        std::shared_ptr<const ClosedPeriod> dropped_latest;
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            dropped.swap(ring_);
            head_ = 0U;
            size_ = 0U;
            dropped_latest = std::atomic_exchange(&latest_, std::shared_ptr<const ClosedPeriod>());
            published_.store(0U, std::memory_order_relaxed);
            run_.fetch_add(1U, std::memory_order_relaxed);
        }
        return run_.load(std::memory_order_relaxed);
    }

    std::uint64_t LiveChannel::run() const noexcept
    {
        return run_.load(std::memory_order_relaxed);
    }

    void LiveChannel::publish(std::shared_ptr<const ClosedPeriod> period)
    {
        std::shared_ptr<const ClosedPeriod> evicted;
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            evicted = std::move(ring_[head_]);
            ring_[head_] = period;
            head_ = (head_ + 1U) % capacity_;
            size_ = std::min(size_ + 1U, capacity_);
            std::atomic_store(&latest_, std::move(period));
            published_.fetch_add(1U, std::memory_order_relaxed);
        }
    }

    std::shared_ptr<const ClosedPeriod> LiveChannel::latest() const noexcept
    {
        return std::atomic_load(&latest_);
    }

    std::vector<std::shared_ptr<const ClosedPeriod>> LiveChannel::history(std::size_t count) const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        const std::size_t wanted = std::min(count, size_);
        std::vector<std::shared_ptr<const ClosedPeriod>> out;
        out.reserve(wanted);
        const std::size_t first = (head_ + capacity_ - wanted) % capacity_;
        for (std::size_t offset = 0U; offset < wanted; ++offset)
            out.push_back(ring_[(first + offset) % capacity_]);
        return out;
    }

    bool LiveChannel::watched() const noexcept
    {
        return viewers_.load(std::memory_order_relaxed) != 0U;
    }

    void LiveChannel::attach() noexcept
    {
        viewers_.fetch_add(1U, std::memory_order_relaxed);
    }

    void LiveChannel::detach() noexcept
    {
        viewers_.fetch_sub(1U, std::memory_order_relaxed);
    }

    std::uint64_t LiveChannel::published() const noexcept
    {
        return published_.load(std::memory_order_relaxed);
    }
} // namespace SushiProfiler
