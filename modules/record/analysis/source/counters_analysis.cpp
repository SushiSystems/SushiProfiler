/**************************************************************************/
/* counters_analysis.cpp                                                  */
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
 * @file counters_analysis.cpp
 * @brief Folds each counter per active period by its operation and summarises the folds.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/counters_analysis.hpp>

#include <string>
#include <utility>
#include <vector>

#include <SushiProfiler/analysis/sample_summary.hpp>

#include "record_queries.hpp"

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns the word for @p op.
        const char* op_name(CounterOp op) noexcept
        {
            switch (op)
            {
                case CounterOp::Add: return "sum";
                case CounterOp::Set: return "last";
                case CounterOp::Max: return "max";
                case CounterOp::Min: return "min";
            }
            return "";
        }
    } // namespace

    std::string_view CountersAnalysis::name() const noexcept
    {
        return "counters";
    }

    Table CountersAnalysis::run(const RunRecord& record, const AnalysisOptions& options) const
    {
        Table table;
        table.name = "counters";
        table.add_column("counter");
        table.add_column("op");
        table.add_column("unit");
        add_summary_columns(table, Unit::None);
        const std::vector<const PeriodRecord*> periods = Detail::active_periods(record);
        for (std::size_t id = 1U; id < record.sites.size(); ++id)
        {
            const SiteRecord& site = record.sites[id];
            if (site.role != SiteRole::Counter)
                continue;
            std::vector<double> folds;
            for (const PeriodRecord* const period : periods)
            {
                double value = 0.0;
                if (Detail::fold_counter(*period, static_cast<SiteID>(id), site.op, value))
                    folds.push_back(value);
            }
            if (folds.empty())
                continue;
            Row row{Cell::text(Detail::site_label(record, static_cast<SiteID>(id))),
                    Cell::text(op_name(site.op)), Cell::text(site.unit)};
            append_summary(row, summarise(folds, options.bootstrap));
            table.rows.push_back(std::move(row));
        }
        return table;
    }
} // namespace SushiProfiler
