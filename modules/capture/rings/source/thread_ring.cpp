/**************************************************************************/
/* thread_ring.cpp                                                        */
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
 * @file thread_ring.cpp
 * @brief Drains a thread's ring and keeps its name behind a sequence counter.
 * @author Mustafa Garip
 * @see modules/capture/rings/README.md
 */

#include <SushiProfiler/rings/thread_ring.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Rounds @p requested up to a power of two of at least the minimum capacity.
        std::size_t round_capacity(std::size_t requested) noexcept
        {
            std::size_t capacity = ThreadRing::K_MIN_CAPACITY;
            while (capacity < requested)
                capacity <<= 1U;
            return capacity;
        }
    } // namespace

    ThreadRing::ThreadRing(std::size_t capacity_events)
        : slots_(new TraceEvent[round_capacity(capacity_events)]),
          mask_(round_capacity(capacity_events) - 1U)
    {
        for (std::atomic<char>& character : name_)
            character.store('\0', std::memory_order_relaxed);
    }

    std::size_t ThreadRing::drain(std::vector<TraceEvent>& out)
    {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t count = head - tail;
        out.reserve(out.size() + count);
        for (std::size_t index = tail; index != head; ++index)
            out.push_back(slots_[index & mask_]);
        tail_.store(head, std::memory_order_release);
        return count;
    }

    std::uint64_t ThreadRing::take_dropped() noexcept
    {
        const std::uint64_t total = dropped_.load(std::memory_order_acquire);
        const std::uint64_t fresh = total - reported_dropped_;
        reported_dropped_ = total;
        return fresh;
    }

    double ThreadRing::fill() const noexcept
    {
        const std::size_t head = head_.load(std::memory_order_acquire);
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        return static_cast<double>(head - tail) / static_cast<double>(mask_ + 1U);
    }

    std::size_t ThreadRing::capacity() const noexcept
    {
        return mask_ + 1U;
    }

    void ThreadRing::set_name(std::string_view name) noexcept
    {
        const std::size_t length =
            name.size() < K_NAME_CAPACITY ? name.size() : K_NAME_CAPACITY - 1U;
        const std::uint32_t sequence = name_sequence_.load(std::memory_order_relaxed);
        name_sequence_.store(sequence + 1U, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);
        for (std::size_t index = 0U; index < K_NAME_CAPACITY; ++index)
            name_[index].store(index < length ? name[index] : '\0', std::memory_order_relaxed);
        name_sequence_.store(sequence + 2U, std::memory_order_release);
    }

    std::string ThreadRing::name() const
    {
        std::array<char, K_NAME_CAPACITY> copy{};
        std::uint32_t before = 0U;
        std::uint32_t after = 0U;
        do
        {
            before = name_sequence_.load(std::memory_order_acquire);
            for (std::size_t index = 0U; index < K_NAME_CAPACITY; ++index)
                copy[index] = name_[index].load(std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_acquire);
            after = name_sequence_.load(std::memory_order_relaxed);
        } while ((before & 1U) != 0U || before != after);
        copy[K_NAME_CAPACITY - 1U] = '\0';
        return std::string(copy.data());
    }
} // namespace SushiProfiler
