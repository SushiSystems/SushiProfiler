/**************************************************************************/
/* thread_ring.hpp                                                        */
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
 * @file thread_ring.hpp
 * @brief Declares one thread's lock-free ring of trace events.
 * @author Mustafa Garip
 * @see modules/capture/rings/README.md
 */

#pragma once

#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/version/export.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#if defined(_MSC_VER)
#pragma warning(push)
// The cache-line alignment of head, tail and name pads the class on purpose.
#pragma warning(disable : 4324)
#endif

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Holds one thread's events between its writes and the consumer's drains.
     *
     * One producer, the owning thread, calls `push`; one consumer calls `drain`, `take_dropped`
     * and `fill`. `push` never blocks, allocates or calls the system: a full ring drops the event
     * and counts it. Head and tail sit on their own cache lines.
     */
    class SUSHIPROFILER_API ThreadRing final
    {
        public:
            /// @brief The smallest capacity a ring is given.
            static constexpr std::size_t K_MIN_CAPACITY = 64U;

            /// @brief The longest thread name kept, its terminator included.
            static constexpr std::size_t K_NAME_CAPACITY = 64U;

            /// @brief Builds a ring of @p capacity_events, rounded up to a power of two.
            explicit ThreadRing(std::size_t capacity_events);

            ThreadRing(const ThreadRing&) = delete;
            ThreadRing& operator=(const ThreadRing&) = delete;
            ThreadRing(ThreadRing&&) = delete;
            ThreadRing& operator=(ThreadRing&&) = delete;
            ~ThreadRing() = default;

            /// @brief Appends @p event; returns false and counts a drop when the ring is full.
            bool push(const TraceEvent& event) noexcept
            {
                const std::size_t head = head_.load(std::memory_order_relaxed);
                if (head - cached_tail_ > mask_)
                {
                    cached_tail_ = tail_.load(std::memory_order_acquire);
                    if (head - cached_tail_ > mask_)
                    {
                        dropped_.store(dropped_.load(std::memory_order_relaxed) + 1U,
                                       std::memory_order_release);
                        return false;
                    }
                }
                slots_[head & mask_] = event;
                head_.store(head + 1U, std::memory_order_release);
                return true;
            }

            /// @brief Appends every held event to @p out, oldest first, and returns how many.
            std::size_t drain(std::vector<TraceEvent>& out);

            /// @brief Returns the drops since the previous call.
            [[nodiscard]] std::uint64_t take_dropped() noexcept;

            /// @brief Returns the held events over the capacity, as the consumer sees them.
            [[nodiscard]] double fill() const noexcept;

            /// @brief Returns the capacity in events.
            [[nodiscard]] std::size_t capacity() const noexcept;

            /// @brief Names the thread, truncated to 63 characters; safe beside `name()`.
            void set_name(std::string_view name) noexcept;

            /// @brief Returns the thread's name.
            [[nodiscard]] std::string name() const;

        private:
            /// @brief The slots, a power of two of them.
            std::unique_ptr<TraceEvent[]> slots_;

            /// @brief The capacity less one.
            std::size_t mask_;

            /// @brief The next slot the producer writes.
            alignas(64) std::atomic<std::size_t> head_{0U};

            /// @brief The producer's last view of tail_.
            std::size_t cached_tail_ = 0U;

            /// @brief Drops since the ring was built; written by the producer only.
            std::atomic<std::uint64_t> dropped_{0U};

            /// @brief The next slot the consumer reads.
            alignas(64) std::atomic<std::size_t> tail_{0U};

            /// @brief Drops already returned by `take_dropped`; the consumer's.
            std::uint64_t reported_dropped_ = 0U;

            /// @brief Odd while the name is being written.
            alignas(64) std::atomic<std::uint32_t> name_sequence_{0U};

            /// @brief The name's characters.
            std::array<std::atomic<char>, K_NAME_CAPACITY> name_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
