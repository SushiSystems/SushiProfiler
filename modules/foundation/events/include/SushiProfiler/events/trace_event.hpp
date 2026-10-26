/**************************************************************************/
/* trace_event.hpp                                                        */
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
 * @file trace_event.hpp
 * @brief Declares the 24-byte event every record kind is written as, and its kinds.
 * @author Mustafa Garip
 * @see modules/foundation/events/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace SushiProfiler
{
    /// @brief Names what an event records, and so what `begin`, `end` and `name` hold.
    enum class EventKind : std::uint8_t
    {
        Zone,            /**< begin and end ticks; name a dynamic name or 0. */
        ExternalZone,    /**< begin and end in the caller's steady ns; name as Zone. */
        Instant,         /**< begin and end the same ticks. */
        ExternalInstant, /**< begin and end the caller's steady ns. */
        Counter,         /**< begin ticks; end the value's bits, folded by the site's operation. */
        Value,           /**< begin ticks; end the value's bits, kept as sampled. */
        PeriodMark,      /**< begin ticks; end the opened period's index; name the timeline. */
        PeriodBudget,    /**< begin ticks; end the period's budget in ns; name the timeline. */
        PeriodTag,       /**< begin ticks; end key << 32 then value; name the timeline. */
        FlowBegin,       /**< begin ticks; end the caller's item id. */
        FlowStep,        /**< begin ticks; end the caller's item id. */
        FlowEnd,         /**< begin ticks; end the caller's item id. */
        LaneBegin,       /**< begin ticks; name the lane. */
        LaneEnd          /**< begin ticks; name the lane. */
    };

    /// @brief The number of kinds; a kind's value is below it.
    inline constexpr std::uint8_t K_EVENT_KIND_COUNT = 14U;

    /// @brief One record in a thread's ring, whatever its kind.
    struct TraceEvent
    {
        std::uint64_t begin;    /**< The first field; see EventKind. */
        std::uint64_t end;      /**< The second field; see EventKind. */
        std::uint32_t site_word; /**< The kind in the top four bits, the site below. */
        NameID name;            /**< The third field; see EventKind. */
    };

    static_assert(sizeof(TraceEvent) == 24U, "a TraceEvent is 24 bytes");
    static_assert(std::is_trivially_copyable_v<TraceEvent>, "a TraceEvent is copied as bytes");

    /// @brief Packs @p kind and @p site into one site word.
    [[nodiscard]] constexpr std::uint32_t pack_site(EventKind kind, SiteID site) noexcept
    {
        return (static_cast<std::uint32_t>(kind) << 28U) | (site & K_MAX_SITE_ID);
    }

    /// @brief Returns the kind of @p event.
    [[nodiscard]] constexpr EventKind kind_of(const TraceEvent& event) noexcept
    {
        return static_cast<EventKind>(event.site_word >> 28U);
    }

    /// @brief Returns the site of @p event.
    [[nodiscard]] constexpr SiteID site_of(const TraceEvent& event) noexcept
    {
        return event.site_word & K_MAX_SITE_ID;
    }

    /// @brief Returns the bits of @p value, NaN payloads and signed zeros included.
    [[nodiscard]] inline std::uint64_t value_bits(double value) noexcept
    {
        std::uint64_t bits = 0U;
        std::memcpy(&bits, &value, sizeof(bits));
        return bits;
    }

    /// @brief Returns the double whose bits are @p bits.
    [[nodiscard]] inline double value_of(std::uint64_t bits) noexcept
    {
        double value = 0.0;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }
} // namespace SushiProfiler
