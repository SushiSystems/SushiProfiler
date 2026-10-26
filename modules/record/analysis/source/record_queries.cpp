/**************************************************************************/
/* record_queries.cpp                                                     */
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
 * @file record_queries.cpp
 * @brief Walks a record's periods, trees and series for the lookups the analyses share.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include "record_queries.hpp"

#include <SushiProfiler/analysis/period_breakdown.hpp>

#include <algorithm>
#include <map>

namespace SushiProfiler::Detail
{
    namespace
    {
        /// @brief One flow point.
        struct Point
        {
            std::uint64_t time_ns = 0U;        /**< When it was recorded. */
            std::uint64_t item = 0U;           /**< Its item. */
            FlowPoint place = FlowPoint::Step; /**< Its place on the item's path. */
        };

        /// @brief Where an item's path stands.
        struct Path
        {
            std::uint64_t begin_ns = 0U; /**< When it began. */
            std::uint64_t last_ns = 0U;  /**< Its latest point. */
            bool begun = false;          /**< Whether its begin was seen. */
        };

        /// @brief Returns the points of flow sites @p sites in @p periods, in time order.
        std::vector<Point> points_of(const std::vector<SiteID>& sites,
                                     const std::vector<const PeriodRecord*>& periods)
        {
            std::vector<Point> points;
            for (const PeriodRecord* const period : periods)
            {
                for (const SampleSeries& series : period->samples)
                {
                    if (std::find(sites.begin(), sites.end(), series.site) == sites.end())
                        continue;
                    for (std::size_t index = 0U; index < series.items.size(); ++index)
                    {
                        points.push_back(Point{series.time_ns[index], series.items[index],
                                               series.points[index]});
                    }
                }
            }
            std::stable_sort(points.begin(), points.end(), [](const Point& left, const Point& right)
                             {
                                 return left.time_ns < right.time_ns;
                             });
            return points;
        }
    } // namespace

    std::vector<NameID> active_timelines(const RunRecord& record)
    {
        std::vector<NameID> timelines;
        for (const PeriodRecord& period : record.periods)
        {
            if (!period.warmup &&
                std::find(timelines.begin(), timelines.end(), period.timeline) == timelines.end())
                timelines.push_back(period.timeline);
        }
        std::sort(timelines.begin(), timelines.end(), [&record](NameID left, NameID right)
                  {
                      return record.name_of(left) < record.name_of(right);
                  });
        return timelines;
    }

    std::vector<const PeriodRecord*> active_periods(const RunRecord& record, NameID timeline)
    {
        std::vector<const PeriodRecord*> periods;
        for (const PeriodRecord& period : record.periods)
        {
            if (!period.warmup && period.timeline == timeline)
                periods.push_back(&period);
        }
        return periods;
    }

    std::vector<const PeriodRecord*> active_periods(const RunRecord& record)
    {
        std::vector<const PeriodRecord*> periods;
        for (const PeriodRecord& period : record.periods)
        {
            if (!period.warmup)
                periods.push_back(&period);
        }
        return periods;
    }

    std::string site_label(const RunRecord& record, SiteID site)
    {
        const SiteRecord* const found = record.site(site);
        if (found == nullptr || found->name.empty())
            return "site " + std::to_string(site);
        return found->name;
    }

    std::string node_label(const RunRecord& record, const ZoneNode& node)
    {
        if (node.name != 0U)
            return std::string(record.name_of(node.name));
        return site_label(record, node.site);
    }

    std::string group_of(const RunRecord& record, const ZoneTree& tree, std::size_t node)
    {
        return group_of_node(record.sites, tree, node);
    }

    bool runs_site(const PeriodRecord& period, SiteID site) noexcept
    {
        for (const ZoneTree& tree : period.zones)
        {
            for (const ZoneNode& node : tree.nodes)
            {
                if (node.site == site)
                    return true;
            }
        }
        return false;
    }

    std::uint64_t site_inclusive(const PeriodRecord& period, SiteID site) noexcept
    {
        std::uint64_t total = 0U;
        for (const ZoneTree& tree : period.zones)
        {
            for (std::size_t index = 0U; index < tree.nodes.size(); ++index)
            {
                if (tree.nodes[index].site != site)
                    continue;
                bool nested = false;
                for (std::int32_t at = tree.nodes[index].parent; at >= 0 && !nested;
                     at = tree.nodes[static_cast<std::size_t>(at)].parent)
                    nested = tree.nodes[static_cast<std::size_t>(at)].site == site;
                if (!nested)
                    total += tree.nodes[index].inclusive_ns;
            }
        }
        return total;
    }

    bool fold_counter(const PeriodRecord& period, SiteID site, CounterOp op, double& out) noexcept
    {
        bool any = false;
        std::uint64_t last_time = 0U;
        for (const SampleSeries& series : period.samples)
        {
            if (series.site != site)
                continue;
            for (std::size_t index = 0U; index < series.values.size(); ++index)
            {
                const double value = series.values[index];
                const std::uint64_t time = series.time_ns[index];
                if (!any)
                    out = op == CounterOp::Add ? 0.0 : value;
                if (op == CounterOp::Add)
                    out += value;
                else if (op == CounterOp::Max)
                    out = std::max(out, value);
                else if (op == CounterOp::Min)
                    out = std::min(out, value);
                else if (!any || time >= last_time)
                    out = value;
                last_time = any ? std::max(last_time, time) : time;
                any = true;
            }
        }
        return any;
    }

    GroupSeries group_series(const RunRecord& record,
                             const std::vector<const PeriodRecord*>& periods)
    {
        GroupSeries series;
        for (std::size_t slot = 0U; slot < periods.size(); ++slot)
        {
            const PeriodBreakdown breakdown = break_down_period(*periods[slot], record.sites);
            for (const GroupSelfTime& entry : breakdown.groups)
            {
                std::vector<double>& group = series.groups[entry.group];
                group.resize(periods.size(), 0.0);
                group[slot] = static_cast<double>(entry.self_ns);
            }
            series.uncovered.push_back(static_cast<double>(breakdown.uncovered_ns));
        }
        return series;
    }

    std::map<std::string, std::vector<SiteID>> flow_sites(const RunRecord& record)
    {
        std::map<std::string, std::vector<SiteID>> sites;
        for (std::size_t id = 1U; id < record.sites.size(); ++id)
        {
            const auto site = static_cast<SiteID>(id);
            if (record.sites[id].role == SiteRole::Flow)
                sites[site_label(record, site)].push_back(site);
        }
        return sites;
    }

    FlowLatencies flow_latencies(const std::vector<SiteID>& sites,
                                 const std::vector<const PeriodRecord*>& periods)
    {
        FlowLatencies latencies;
        std::map<std::uint64_t, Path> paths;
        for (const Point& point : points_of(sites, periods))
        {
            const auto found = paths.find(point.item);
            if (point.place == FlowPoint::Begin)
            {
                paths[point.item] = Path{point.time_ns, point.time_ns, true};
                continue;
            }
            if (found == paths.end())
            {
                paths[point.item] = Path{0U, point.time_ns, false};
                continue;
            }
            latencies.steps.push_back(static_cast<double>(point.time_ns - found->second.last_ns));
            found->second.last_ns = point.time_ns;
            if (point.place != FlowPoint::End)
                continue;
            if (found->second.begun)
            {
                const std::uint64_t total = point.time_ns - found->second.begin_ns;
                latencies.totals.push_back(static_cast<double>(total));
            }
            paths.erase(found);
        }
        return latencies;
    }

    std::vector<double> durations(const std::vector<const PeriodRecord*>& periods)
    {
        std::vector<double> lengths;
        lengths.reserve(periods.size());
        for (const PeriodRecord* const period : periods)
            lengths.push_back(static_cast<double>(period->duration_ns()));
        return lengths;
    }
} // namespace SushiProfiler::Detail
