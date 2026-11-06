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
 * @file flows_analysis.cpp
 * @brief Follows each item's points in time order and measures its total and step latencies.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/flows_analysis.hpp>

#include <string>
#include <utility>
#include <vector>

#include <SushiProfiler/analysis/sample_summary.hpp>

#include "record_queries.hpp"

namespace SushiProfiler
{
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
        for (const auto& [label, sites] : Detail::flow_sites(record))
        {
            const Detail::FlowLatencies latencies = Detail::flow_latencies(sites, periods);
            const std::vector<double>& totals = latencies.totals;
            const std::vector<double>& steps = latencies.steps;
            for (const auto& [segment, samples] : {std::make_pair("total", &totals),
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
