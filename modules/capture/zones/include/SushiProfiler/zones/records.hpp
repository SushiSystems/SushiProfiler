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
        /// @brief Writes one event of @p kind at @p handle's site into @p collector if it records.
        inline void record(Collector& collector, SiteHandle& handle, EventKind kind,
                           std::uint64_t begin, std::uint64_t end, NameID name) noexcept
        {
            SiteID site = 0U;
            ThreadRing* const ring = admit(collector, handle, site);
            if (ring != nullptr)
                static_cast<void>(ring->push(TraceEvent{begin, end, pack_site(kind, site), name}));
        }

        /// @brief Writes an event of @p kind stamped now, carrying @p end, if @p collector records.
        inline void record_now(Collector& collector, SiteHandle& handle, EventKind kind,
                               std::uint64_t end) noexcept
        {
            SiteID site = 0U;
            ThreadRing* const ring = admit(collector, handle, site);
            if (ring == nullptr)
                return;
            const TraceEvent event{collector.stamp(), end, pack_site(kind, site), 0U};
            static_cast<void>(ring->push(event));
        }
    } // namespace Detail

    /// @brief Records @p value at a counter site of @p collector; the site's operation folds it.
    inline void record_counter(Collector& collector, SiteHandle& handle, double value) noexcept
    {
        Detail::record_now(collector, handle, EventKind::Counter, value_bits(value));
    }

    /// @brief Records @p value at a counter site of the process-wide collector.
    inline void record_counter(SiteHandle& handle, double value) noexcept
    {
        if (Collector::recording())
            record_counter(Collector::instance(), handle, value);
    }

    /// @brief Records @p value at a value site of @p collector, kept as sampled.
    inline void record_value(Collector& collector, SiteHandle& handle, double value) noexcept
    {
        Detail::record_now(collector, handle, EventKind::Value, value_bits(value));
    }

    /// @brief Records @p value at a value site of the process-wide collector.
    inline void record_value(SiteHandle& handle, double value) noexcept
    {
        if (Collector::recording())
            record_value(Collector::instance(), handle, value);
    }

    /// @brief Records an occurrence now in @p collector.
    inline void record_instant(Collector& collector, SiteHandle& handle) noexcept
    {
        SiteID site = 0U;
        ThreadRing* const ring = Detail::admit(collector, handle, site);
        if (ring == nullptr)
            return;
        const std::uint64_t now = collector.stamp();
        const TraceEvent event{now, now, pack_site(EventKind::Instant, site), 0U};
        static_cast<void>(ring->push(event));
    }

    /// @brief Records an occurrence now in the process-wide collector.
    inline void record_instant(SiteHandle& handle) noexcept
    {
        if (Collector::recording())
            record_instant(Collector::instance(), handle);
    }

    /// @brief Records in @p collector an occurrence at @p steady_ns, read on the steady clock.
    inline void record_instant_at(Collector& collector, SiteHandle& handle,
                                  std::uint64_t steady_ns) noexcept
    {
        Detail::record(collector, handle, EventKind::ExternalInstant, steady_ns, steady_ns, 0U);
    }

    /// @brief Records an occurrence at @p steady_ns, a time the caller read on the steady clock.
    inline void record_instant_at(SiteHandle& handle, std::uint64_t steady_ns) noexcept
    {
        if (Collector::recording())
            record_instant_at(Collector::instance(), handle, steady_ns);
    }

    /**
     * @brief Records in @p collector a time range the caller measured on the steady clock.
     * @param name A dynamic name, or 0 for the site's.
     */
    inline void record_zone_at(Collector& collector, SiteHandle& handle, std::uint64_t begin_ns,
                               std::uint64_t end_ns, NameID name = 0U) noexcept
    {
        Detail::record(collector, handle, EventKind::ExternalZone, begin_ns, end_ns, name);
    }

    /**
     * @brief Records a time range the caller measured on the steady clock, such as a device span.
     * @param name A dynamic name, or 0 for the site's.
     */
    inline void record_zone_at(SiteHandle& handle, std::uint64_t begin_ns, std::uint64_t end_ns,
                               NameID name = 0U) noexcept
    {
        if (Collector::recording())
            record_zone_at(Collector::instance(), handle, begin_ns, end_ns, name);
    }

    /**
     * @brief Records in @p collector a point on item @p id's path across threads.
     * @pre @p kind is FlowBegin, FlowStep or FlowEnd.
     */
    inline void record_flow(Collector& collector, SiteHandle& handle, EventKind kind,
                            std::uint64_t id) noexcept
    {
        Detail::record_now(collector, handle, kind, id);
    }

    /**
     * @brief Records a point on item @p id's path across threads.
     * @pre @p kind is FlowBegin, FlowStep or FlowEnd.
     */
    inline void record_flow(SiteHandle& handle, EventKind kind, std::uint64_t id) noexcept
    {
        if (Collector::recording())
            record_flow(Collector::instance(), handle, kind, id);
    }
} // namespace SushiProfiler
