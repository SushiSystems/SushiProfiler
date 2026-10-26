/**************************************************************************/
/* analysis_record.cpp                                                    */
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
 * @file analysis_record.cpp
 * @brief Builds the analysis tests' record and compares rows cell by cell.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include "analysis_record.hpp"

#include <gtest/gtest.h>

#include <cmath>

using SushiProfiler::Cell;
using SushiProfiler::CounterOp;
using SushiProfiler::FlowPoint;
using SushiProfiler::PeriodRecord;
using SushiProfiler::Row;
using SushiProfiler::RunRecord;
using SushiProfiler::SampleSeries;
using SushiProfiler::SiteRecord;
using SushiProfiler::SiteRole;
using SushiProfiler::ThreadRecord;
using SushiProfiler::ZoneNode;
using SushiProfiler::ZoneTree;

namespace
{
    /// @brief Returns a site named @p name in @p group with @p role, @p op and @p unit.
    SiteRecord site(const char* name, const char* group, SiteRole role,
                    CounterOp op = CounterOp::Add, const char* unit = "")
    {
        SiteRecord site;
        site.name = name;
        site.group = group;
        site.role = role;
        site.op = op;
        site.unit = unit;
        return site;
    }

    /// @brief Returns a series of @p site on thread 0 with @p times and @p values.
    SampleSeries series(SushiProfiler::SiteID site, std::vector<std::uint64_t> times,
                        std::vector<double> values)
    {
        SampleSeries series;
        series.site = site;
        series.time_ns = std::move(times);
        series.values = std::move(values);
        return series;
    }

    /// @brief Returns flow points of site 7 on thread 0.
    SampleSeries flow(std::vector<std::uint64_t> times, std::vector<std::uint64_t> items,
                      std::vector<FlowPoint> points)
    {
        SampleSeries series;
        series.site = 7U;
        series.time_ns = std::move(times);
        series.items = std::move(items);
        series.points = std::move(points);
        return series;
    }

    /// @brief Returns period @p index of timeline 1 from @p begin to @p end.
    PeriodRecord period(std::uint64_t index, std::uint64_t begin, std::uint64_t end)
    {
        PeriodRecord period;
        period.index = index;
        period.timeline = 1U;
        period.begin_ns = begin;
        period.end_ns = end;
        period.budget_ns = 105U;
        return period;
    }

    /// @brief Returns a tree on thread 0 holding @p nodes.
    ZoneTree tree(std::vector<ZoneNode> nodes)
    {
        ZoneTree tree;
        tree.nodes = std::move(nodes);
        return tree;
    }
} // namespace

namespace SushiProfilerTests
{
    RunRecord analysis_record()
    {
        RunRecord record;
        record.clock.read_cost_ns = 6.5;
        record.names = {"", "step"};
        record.sites = {SiteRecord{},
                        site("forward", "Model", SiteRole::Zone),
                        site("conv", "", SiteRole::Zone),
                        site("load", "Data", SiteRole::Zone),
                        site("bytes", "", SiteRole::Counter, CounterOp::Add, "B"),
                        site("peak", "", SiteRole::Counter, CounterOp::Max, "B"),
                        site("loss", "", SiteRole::Value),
                        site("item", "", SiteRole::Flow)};
        record.threads = {ThreadRecord{0U, "main", 2U, 1U}};
        record.drops = {1U, 2U, 3U};

        PeriodRecord warmup = period(0U, 0U, 100U);
        warmup.warmup = true;
        warmup.budget_ns = 0U;
        warmup.zones = {tree({ZoneNode{1U, 0U, -1, 99U, 99U, 1U}})};

        PeriodRecord first = period(1U, 100U, 200U);
        first.zones = {tree({ZoneNode{1U, 0U, -1, 60U, 40U, 1U}, ZoneNode{2U, 0U, 0, 20U, 20U, 1U},
                             ZoneNode{3U, 0U, -1, 30U, 30U, 1U}})};
        first.samples = {series(4U, {101U, 102U}, {100.0, 50.0}),
                         series(5U, {103U, 104U, 105U}, {5.0, 9.0, 3.0}),
                         series(6U, {106U, 107U}, {1.0, 3.0}),
                         flow({110U, 130U}, {1U, 1U}, {FlowPoint::Begin, FlowPoint::Step})};

        PeriodRecord second = period(2U, 200U, 320U);
        second.zones = {tree({ZoneNode{1U, 0U, -1, 80U, 50U, 1U}, ZoneNode{2U, 0U, 0, 30U, 30U, 1U},
                              ZoneNode{3U, 0U, -1, 20U, 20U, 1U}})};
        second.samples = {series(4U, {201U}, {200.0}), series(5U, {202U}, {4.0}),
                          series(6U, {203U}, {2.0}),
                          flow({210U, 250U}, {1U, 2U}, {FlowPoint::End, FlowPoint::Begin})};

        PeriodRecord third = period(3U, 320U, 430U);
        third.zones = {tree({ZoneNode{1U, 0U, -1, 70U, 70U, 1U},
                             ZoneNode{3U, 0U, -1, 30U, 30U, 1U}})};
        third.samples = {series(5U, {321U}, {7.0}),
                         flow({330U, 340U}, {2U, 2U}, {FlowPoint::Step, FlowPoint::End})};

        record.periods = {warmup, first, second, third};
        return record;
    }

    Cell text(const char* value)
    {
        return Cell::text(value);
    }

    Cell whole(std::int64_t value)
    {
        return Cell::whole(value);
    }

    Cell real(double value)
    {
        return Cell::real(value);
    }

    void expect_row(const Row& row, const Row& expected)
    {
        ASSERT_EQ(row.size(), expected.size());
        for (std::size_t index = 0U; index < row.size(); ++index)
        {
            const double* const real = row[index].as_real();
            const double* const wanted = expected[index].as_real();
            if (real != nullptr && wanted != nullptr)
            {
                const double tolerance = 1e-9 * std::fabs(*wanted) + 1e-12;
                EXPECT_NEAR(*real, *wanted, tolerance) << "column " << index;
            }
            else
            {
                EXPECT_EQ(row[index], expected[index]) << "column " << index;
            }
        }
    }
} // namespace SushiProfilerTests
