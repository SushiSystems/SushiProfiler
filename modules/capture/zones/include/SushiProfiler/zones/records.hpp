/**************************************************************************/
/* records.hpp                                                            */
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
 * @file records.hpp
 * @brief Declares the one-shot records: counters, values, instants, external zones and flows.
 * @author Mustafa Garip
 * @see modules/capture/zones/README.md
 */

#pragma once

#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/rings/thread_ring.hpp>
#include <SushiProfiler/sites/site.hpp>
#include <SushiProfiler/zones/admission.hpp>

#include <cstdint>

namespace SushiProfiler
{
    namespace Detail
    {
        /// @brief Writes one event of @p kind at @p handle's site if it records now.
        inline void record(SiteHandle& handle, EventKind kind, std::uint64_t begin,
                           std::uint64_t end, NameID name) noexcept
        {
            SiteID site = 0U;
            ThreadRing* const ring = admit(handle, site);
            if (ring != nullptr)
                static_cast<void>(ring->push(TraceEvent{begin, end, pack_site(kind, site), name}));
        }

        /// @brief Writes one event of @p kind stamped now, carrying @p end, if it records.
        inline void record_now(SiteHandle& handle, EventKind kind, std::uint64_t end) noexcept
        {
            SiteID site = 0U;
            ThreadRing* const ring = admit(handle, site);
            if (ring == nullptr)
                return;
            const TraceEvent event{Collector::now(), end, pack_site(kind, site), 0U};
            static_cast<void>(ring->push(event));
        }
    } // namespace Detail

    /// @brief Records @p value at a counter site; the site's operation folds it per period.
    inline void record_counter(SiteHandle& handle, double value) noexcept
    {
        Detail::record_now(handle, EventKind::Counter, value_bits(value));
    }

    /// @brief Records @p value at a value site, kept as sampled.
    inline void record_value(SiteHandle& handle, double value) noexcept
    {
        Detail::record_now(handle, EventKind::Value, value_bits(value));
    }

    /// @brief Records an occurrence now.
    inline void record_instant(SiteHandle& handle) noexcept
    {
        SiteID site = 0U;
        ThreadRing* const ring = Detail::admit(handle, site);
        if (ring == nullptr)
            return;
        const std::uint64_t now = Collector::now();
        static_cast<void>(ring->push(TraceEvent{now, now, pack_site(EventKind::Instant, site), 0U}));
    }

    /// @brief Records an occurrence at @p steady_ns, a time the caller read on the steady clock.
    inline void record_instant_at(SiteHandle& handle, std::uint64_t steady_ns) noexcept
    {
        Detail::record(handle, EventKind::ExternalInstant, steady_ns, steady_ns, 0U);
    }

    /**
     * @brief Records a time range the caller measured on the steady clock, such as a device span.
     * @param name A dynamic name, or 0 for the site's.
     */
    inline void record_zone_at(SiteHandle& handle, std::uint64_t begin_ns, std::uint64_t end_ns,
                               NameID name = 0U) noexcept
    {
        Detail::record(handle, EventKind::ExternalZone, begin_ns, end_ns, name);
    }

    /**
     * @brief Records a point on item @p id's path across threads.
     * @pre @p kind is FlowBegin, FlowStep or FlowEnd.
     */
    inline void record_flow(SiteHandle& handle, EventKind kind, std::uint64_t id) noexcept
    {
        Detail::record_now(handle, kind, id);
    }
} // namespace SushiProfiler
