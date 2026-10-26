/**************************************************************************/
/* period_fold.cpp                                                        */
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
 * @file period_fold.cpp
 * @brief Sorts each thread and lane's zones into nesting order and merges them into trees.
 * @author Mustafa Garip
 * @see modules/session/fold/README.md
 */

#include <SushiProfiler/fold/period_fold.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <tuple>
#include <utility>

namespace SushiProfiler
{
    namespace
    {
        /// @brief One zone run as the fold sorts it.
        struct Interval
        {
            std::uint64_t begin = 0U; /**< When it began. */
            std::uint64_t end = 0U;   /**< When it ended. */
            SiteID site = 0U;         /**< Its site. */
            NameID name = 0U;         /**< Its dynamic name. */
            std::size_t order = 0U;   /**< Its place in the ring. */

            /// @brief Returns its length.
            [[nodiscard]] std::uint64_t length() const noexcept
            {
                return end - begin;
            }
        };

        /// @brief Returns whether @p left opens before @p right: earlier, then longer, then later.
        bool opens_before(const Interval& left, const Interval& right) noexcept
        {
            if (left.begin != right.begin)
                return left.begin < right.begin;
            if (left.end != right.end)
                return left.end > right.end;
            return left.order > right.order;
        }

        /// @brief Each run's parent run, -1 at the root, with its depth and its children's time.
        struct Nesting
        {
            std::vector<std::int64_t> parent;        /**< The parent run, or -1. */
            std::vector<std::uint32_t> depth;        /**< Runs open around it. */
            std::vector<std::uint64_t> children_ns;  /**< Time inside its direct children. */
            std::uint64_t unmatched = 0U;            /**< Runs that ended after their parent. */
        };

        /// @brief Nests @p zones, sorted by opens_before(), by time.
        Nesting nest(const std::vector<Interval>& zones)
        {
            Nesting nesting;
            nesting.parent.assign(zones.size(), -1);
            nesting.depth.assign(zones.size(), 0U);
            nesting.children_ns.assign(zones.size(), 0U);
            std::vector<std::size_t> open;
            for (std::size_t index = 0U; index < zones.size(); ++index)
            {
                const Interval& zone = zones[index];
                bool crossed = false;
                while (!open.empty())
                {
                    const Interval& outer = zones[open.back()];
                    if (outer.end > zone.begin && zone.end <= outer.end)
                        break;
                    crossed = crossed || outer.end > zone.begin;
                    open.pop_back();
                }
                nesting.unmatched += crossed ? 1U : 0U;
                if (!open.empty())
                {
                    nesting.parent[index] = static_cast<std::int64_t>(open.back());
                    nesting.children_ns[open.back()] += zone.length();
                }
                nesting.depth[index] = static_cast<std::uint32_t>(open.size());
                open.push_back(index);
            }
            return nesting;
        }

        /// @brief Folds one thread and lane's @p zones into a tree and its instances.
        std::uint64_t fold_zones(std::uint32_t thread, NameID lane, std::vector<Interval>& zones,
                                 PeriodFold& out)
        {
            std::sort(zones.begin(), zones.end(), opens_before);
            const Nesting nesting = nest(zones);
            ZoneTree tree;
            tree.thread = thread;
            tree.lane = lane;
            std::map<std::tuple<std::int32_t, SiteID, NameID>, std::int32_t> nodes;
            std::vector<std::int32_t> node_of(zones.size(), -1);
            for (std::size_t index = 0U; index < zones.size(); ++index)
            {
                const Interval& zone = zones[index];
                const std::int64_t parent = nesting.parent[index];
                const std::int32_t parent_node =
                    parent < 0 ? -1 : node_of[static_cast<std::size_t>(parent)];
                const auto key = std::make_tuple(parent_node, zone.site, zone.name);
                auto found = nodes.find(key);
                if (found == nodes.end())
                {
                    ZoneNode node;
                    node.site = zone.site;
                    node.name = zone.name;
                    node.parent = parent_node;
                    tree.nodes.push_back(node);
                    const auto added = static_cast<std::int32_t>(tree.nodes.size() - 1U);
                    found = nodes.emplace(key, added).first;
                }
                ZoneNode& node = tree.nodes[static_cast<std::size_t>(found->second)];
                node.calls += 1U;
                node.inclusive_ns += zone.length();
                node.self_ns += zone.length() - std::min(zone.length(), nesting.children_ns[index]);
                node_of[index] = found->second;
                out.instances.push_back(ZoneInstance{thread, lane, zone.site, zone.name,
                                                     zone.begin, zone.end, nesting.depth[index]});
            }
            out.zones.push_back(std::move(tree));
            return nesting.unmatched;
        }

        /// @brief Appends @p event to @p series when it is a sample, flow point or instant.
        void add_sample(SampleSeries& series, const TraceEvent& event, EventKind kind)
        {
            series.time_ns.push_back(event.begin);
            if (kind == EventKind::Counter || kind == EventKind::Value)
                series.values.push_back(value_of(event.end));
            if (kind == EventKind::FlowBegin || kind == EventKind::FlowStep ||
                kind == EventKind::FlowEnd)
            {
                series.items.push_back(event.end);
                series.points.push_back(kind == EventKind::FlowBegin  ? FlowPoint::Begin
                                        : kind == EventKind::FlowStep ? FlowPoint::Step
                                                                      : FlowPoint::End);
            }
        }

        /// @brief Returns whether events of @p kind become samples.
        bool is_sample(EventKind kind) noexcept
        {
            switch (kind)
            {
                case EventKind::Instant:
                case EventKind::ExternalInstant:
                case EventKind::Counter:
                case EventKind::Value:
                case EventKind::FlowBegin:
                case EventKind::FlowStep:
                case EventKind::FlowEnd:
                    return true;
                default:
                    return false;
            }
        }
    } // namespace

    PeriodFold fold_period(const std::vector<ThreadEvents>& threads)
    {
        std::map<std::pair<std::uint32_t, NameID>, std::vector<Interval>> zones;
        std::map<std::pair<SiteID, std::uint32_t>, SampleSeries> samples;
        std::map<std::uint32_t, std::uint64_t> unmatched;
        for (const ThreadEvents& thread : threads)
        {
            for (std::size_t order = 0U; order < thread.events.size(); ++order)
            {
                const TraceEvent& event = thread.events[order].event;
                const EventKind kind = kind_of(event);
                const SiteID site = site_of(event);
                if (kind == EventKind::Zone || kind == EventKind::ExternalZone)
                {
                    if (event.end < event.begin)
                        unmatched[thread.thread] += 1U;
                    else
                        zones[{thread.thread, thread.events[order].lane}].push_back(
                            Interval{event.begin, event.end, site, event.name, order});
                    continue;
                }
                if (!is_sample(kind))
                    continue;
                SampleSeries& series = samples[{site, thread.thread}];
                series.site = site;
                series.thread = thread.thread;
                add_sample(series, event, kind);
            }
        }
        PeriodFold fold;
        for (auto& [key, intervals] : zones)
        {
            const std::uint64_t crossed = fold_zones(key.first, key.second, intervals, fold);
            if (crossed != 0U)
                unmatched[key.first] += crossed;
        }
        for (auto& entry : samples)
            fold.samples.push_back(std::move(entry.second));
        for (const auto& [thread, count] : unmatched)
            fold.unmatched.push_back(ThreadCount{thread, count});
        return fold;
    }
} // namespace SushiProfiler
