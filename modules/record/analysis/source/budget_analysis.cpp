/**************************************************************************/
/* budget_analysis.cpp                                                    */
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
 * @file budget_analysis.cpp
 * @brief Divides each budgeted period's length by its budget and finds the worst period.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/budget_analysis.hpp>

#include <string>
#include <utility>
#include <vector>

#include <SushiProfiler/statistics/sorted_samples.hpp>

#include "record_queries.hpp"

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns the label of the node with the most self time on @p period's thread.
        std::string heaviest_zone(const RunRecord& record, const PeriodRecord& period)
        {
            const ZoneNode* heaviest = nullptr;
            for (const ZoneTree& tree : period.zones)
            {
                if (tree.thread != period.thread)
                    continue;
                for (const ZoneNode& node : tree.nodes)
                {
                    if (heaviest == nullptr || node.self_ns > heaviest->self_ns)
                        heaviest = &node;
                }
            }
            return heaviest == nullptr ? std::string() : Detail::node_label(record, *heaviest);
        }
    } // namespace

    std::string_view BudgetAnalysis::name() const noexcept
    {
        return "budget";
    }

    Table BudgetAnalysis::run(const RunRecord& record, const AnalysisOptions& options) const
    {
        static_cast<void>(options);
        Table table;
        table.name = "budget";
        table.add_column("timeline");
        table.add_column("periods", Unit::Count);
        table.add_column("used_median", Unit::Ratio);
        table.add_column("over", Unit::Count);
        table.add_column("worst_period", Unit::Count);
        table.add_column("worst_used", Unit::Ratio);
        table.add_column("worst_zone");
        for (const NameID timeline : Detail::active_timelines(record))
        {
            std::vector<double> used;
            std::int64_t over = 0;
            const PeriodRecord* worst = nullptr;
            double worst_used = 0.0;
            for (const PeriodRecord* const period : Detail::active_periods(record, timeline))
            {
                if (period->budget_ns == 0U)
                    continue;
                const double fraction = static_cast<double>(period->duration_ns()) /
                                        static_cast<double>(period->budget_ns);
                used.push_back(fraction);
                over += fraction > 1.0 ? 1 : 0;
                if (worst == nullptr || fraction > worst_used)
                {
                    worst = period;
                    worst_used = fraction;
                }
            }
            if (worst == nullptr)
                continue;
            Row row{Cell::text(std::string(record.name_of(timeline))),
                    Cell::whole(static_cast<std::int64_t>(used.size())),
                    Cell::real(SortedSamples(used).median()), Cell::whole(over),
                    Cell::whole(static_cast<std::int64_t>(worst->index)), Cell::real(worst_used),
                    Cell::text(heaviest_zone(record, *worst))};
            table.rows.push_back(std::move(row));
        }
        return table;
    }
} // namespace SushiProfiler
