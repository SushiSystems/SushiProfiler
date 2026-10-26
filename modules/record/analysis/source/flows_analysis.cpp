/**************************************************************************/
/* flows_analysis.cpp                                                     */
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
 * @file flows_analysis.cpp
 * @brief Follows each item's points in time order and measures its total and step latencies.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/flows_analysis.hpp>

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <SushiProfiler/analysis/sample_summary.hpp>

#include "record_queries.hpp"

namespace SushiProfiler
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

        /// @brief Returns the points of flow site @p site in @p periods, in time order.
        std::vector<Point> points_of(SiteID site, const std::vector<const PeriodRecord*>& periods)
        {
            std::vector<Point> points;
            for (const PeriodRecord* const period : periods)
            {
                for (const SampleSeries& series : period->samples)
                {
                    if (series.site != site)
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

    std::string_view FlowsAnalysis::name() const noexcept
    {
        return "flows";
    }

    Table FlowsAnalysis::run(const RunRecord& record, const AnalysisOptions& options) const
    {
        Table table;
        table.name = "flows";
        table.add_column("flow");
        table.add_column("segment");
        add_summary_columns(table, Unit::Nanoseconds);
        const std::vector<const PeriodRecord*> periods = Detail::active_periods(record);
        for (std::size_t id = 1U; id < record.sites.size(); ++id)
        {
            if (record.sites[id].role != SiteRole::Flow)
                continue;
            std::map<std::uint64_t, Path> paths;
            std::vector<double> totals;
            std::vector<double> steps;
            for (const Point& point : points_of(static_cast<SiteID>(id), periods))
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
                steps.push_back(static_cast<double>(point.time_ns - found->second.last_ns));
                found->second.last_ns = point.time_ns;
                if (point.place != FlowPoint::End)
                    continue;
                if (found->second.begun)
                    totals.push_back(static_cast<double>(point.time_ns - found->second.begin_ns));
                paths.erase(found);
            }
            const std::string label = Detail::site_label(record, static_cast<SiteID>(id));
            for (auto& [segment, samples] : {std::make_pair("total", &totals),
                                             std::make_pair("step", &steps)})
            {
                if (samples->empty())
                    continue;
                Row row{Cell::text(label), Cell::text(segment)};
                append_summary(row, summarise(*samples, options.bootstrap));
                table.rows.push_back(std::move(row));
            }
        }
        return table;
    }
} // namespace SushiProfiler
