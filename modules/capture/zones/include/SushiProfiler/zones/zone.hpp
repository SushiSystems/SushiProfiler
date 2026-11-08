/**************************************************************************/
/* zone.hpp                                                               */
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
 * @file zone.hpp
 * @brief Declares the scoped zone and the scoped lane, each compiled in or out by a flag.
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
#include <type_traits>

namespace SushiProfiler
{
    /// @brief Stands in for a site handle where the record is compiled out.
    struct CompiledOutHandle
    {
        /// @brief Ignores @p site, so a compiled-out record keeps no handle.
        constexpr explicit CompiledOutHandle(const Site* site) noexcept
        {
            static_cast<void>(site);
        }
    };

    /// @brief The handle a record keeps: a site handle when compiled in, an empty one when out.
    template <bool Compiled>
    using SiteSlot = std::conditional_t<Compiled, SiteHandle, CompiledOutHandle>;

    /// @brief Records the time its scope takes as one Zone event; @p Compiled false is empty.
    template <bool Compiled>
    class ScopedZone;

    /// @brief A compiled-out zone: empty, and its constructor does nothing.
    template <>
    class ScopedZone<false>
    {
        public:
            /// @brief Ignores its arguments.
            template <class... Arguments>
            constexpr explicit ScopedZone(const Arguments&...) noexcept
            {

            }
    };

    /// @brief A compiled-in zone: reads the clock on entry and writes one event on exit.
    template <>
    class ScopedZone<true>
    {
        public:
            /**
             * @brief Starts a zone at @p handle's site in the process-wide collector, named
             *        @p name when it is not 0.
             */
            explicit ScopedZone(SiteHandle& handle, NameID name = 0U) noexcept
            {
                if (Collector::recording())
                    start(Collector::instance(), handle, name);
            }

            /// @brief Starts a zone at @p handle's site in @p collector, named @p name when not 0.
            ScopedZone(Collector& collector, SiteHandle& handle, NameID name = 0U) noexcept
            {
                start(collector, handle, name);
            }

            ScopedZone(const ScopedZone&) = delete;
            ScopedZone& operator=(const ScopedZone&) = delete;
            ScopedZone(ScopedZone&&) = delete;
            ScopedZone& operator=(ScopedZone&&) = delete;

            /// @brief Writes the zone if it started and the collector still records.
            ~ScopedZone()
            {
                if (ring_ == nullptr || !collector_->is_recording())
                    return;
                const TraceEvent event{begin_, collector_->stamp(), site_word_, name_};
                static_cast<void>(ring_->push(event));
            }

        private:
            /// @brief Admits the zone into @p collector and reads its start.
            void start(Collector& collector, SiteHandle& handle, NameID name) noexcept
            {
                SiteID site = 0U;
                ring_ = Detail::admit(collector, handle, site);
                if (ring_ == nullptr)
                    return;
                collector_ = &collector;
                site_word_ = pack_site(EventKind::Zone, site);
                name_ = name;
                begin_ = collector.stamp();
            }

            /// @brief The collector the zone records into, set when the ring is.
            Collector* collector_ = nullptr;

            /// @brief The ring to write into, or null when the zone does not record.
            ThreadRing* ring_ = nullptr;

            /// @brief The time the zone started.
            std::uint64_t begin_ = 0U;

            /// @brief The kind and the site.
            std::uint32_t site_word_ = 0U;

            /// @brief The dynamic name, or 0.
            NameID name_ = 0U;
    };

    /// @brief Brackets its scope with LaneBegin and LaneEnd; @p Compiled false is empty.
    template <bool Compiled>
    class ScopedLane;

    /// @brief A compiled-out lane: empty, and its constructor does nothing.
    template <>
    class ScopedLane<false>
    {
        public:
            /// @brief Ignores its arguments.
            template <class... Arguments>
            constexpr explicit ScopedLane(const Arguments&...) noexcept
            {

            }
    };

    /// @brief A compiled-in lane: everything its thread records inside belongs to the lane.
    template <>
    class ScopedLane<true>
    {
        public:
            /// @brief Opens lane @p lane, an interned name, if the process-wide collector records.
            explicit ScopedLane(NameID lane) noexcept : lane_(lane)
            {
                if (Collector::recording())
                    start(Collector::instance());
            }

            /// @brief Opens lane @p lane, an interned name, if @p collector records.
            ScopedLane(Collector& collector, NameID lane) noexcept : lane_(lane)
            {
                start(collector);
            }

            ScopedLane(const ScopedLane&) = delete;
            ScopedLane& operator=(const ScopedLane&) = delete;
            ScopedLane(ScopedLane&&) = delete;
            ScopedLane& operator=(ScopedLane&&) = delete;

            /// @brief Closes the lane if it opened and the collector still records.
            ~ScopedLane()
            {
                if (ring_ == nullptr || !collector_->is_recording())
                    return;
                const std::uint64_t now = collector_->stamp();
                const TraceEvent event{now, now, pack_site(EventKind::LaneEnd, 0U), lane_};
                static_cast<void>(ring_->push(event));
            }

        private:
            /// @brief Writes the lane's begin into @p collector if it records.
            void start(Collector& collector) noexcept
            {
                if (!collector.is_recording())
                    return;
                ring_ = collector.thread_ring();
                if (ring_ == nullptr)
                    return;
                collector_ = &collector;
                const std::uint64_t now = collector.stamp();
                const TraceEvent event{now, now, pack_site(EventKind::LaneBegin, 0U), lane_};
                static_cast<void>(ring_->push(event));
            }

            /// @brief The collector the lane records into, set when the ring is.
            Collector* collector_ = nullptr;

            /// @brief The ring to write into, or null when the lane did not open.
            ThreadRing* ring_ = nullptr;

            /// @brief The lane's name.
            NameID lane_;
    };
} // namespace SushiProfiler
