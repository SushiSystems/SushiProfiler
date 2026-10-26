/**************************************************************************/
/* builtin_analyses.cpp                                                   */
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
 * @file builtin_analyses.cpp
 * @brief Adds every analysis SushiProfiler ships to a catalog.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/analysis_catalog.hpp>

#include <memory>

#include <SushiProfiler/analysis/breakdown_analysis.hpp>
#include <SushiProfiler/analysis/budget_analysis.hpp>
#include <SushiProfiler/analysis/counters_analysis.hpp>
#include <SushiProfiler/analysis/device_idle_analysis.hpp>
#include <SushiProfiler/analysis/distribution_analysis.hpp>
#include <SushiProfiler/analysis/flows_analysis.hpp>
#include <SushiProfiler/analysis/overhead_analysis.hpp>
#include <SushiProfiler/analysis/roofline_analysis.hpp>
#include <SushiProfiler/analysis/short_spans_analysis.hpp>
#include <SushiProfiler/analysis/top_spans_analysis.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns a new analysis of type Kind.
        template <class Kind>
        std::unique_ptr<Analysis> make_analysis()
        {
            return std::make_unique<Kind>();
        }

        /// @brief Adds the analysis of type Kind under the name it reports.
        template <class Kind>
        void add(AnalysisCatalog& catalog)
        {
            static_cast<void>(catalog.add(std::string(Kind().name()), &make_analysis<Kind>));
        }
    } // namespace

    void register_builtin_analyses(AnalysisCatalog& catalog)
    {
        add<BreakdownAnalysis>(catalog);
        add<BudgetAnalysis>(catalog);
        add<CountersAnalysis>(catalog);
        add<DeviceIdleAnalysis>(catalog);
        add<DistributionAnalysis>(catalog);
        add<FlowsAnalysis>(catalog);
        add<OverheadAnalysis>(catalog);
        add<RooflineAnalysis>(catalog);
        add<ShortSpansAnalysis>(catalog);
        add<TopSpansAnalysis>(catalog);
    }
} // namespace SushiProfiler
