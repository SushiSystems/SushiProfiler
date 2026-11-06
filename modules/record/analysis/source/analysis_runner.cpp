/**************************************************************************/
/* analysis_runner.cpp                                                    */
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
 * @file analysis_runner.cpp
 * @brief Checks every name, then runs each analysis in the order asked.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/analysis_runner.hpp>

#include <memory>

namespace SushiProfiler
{
    AnalysisRunner::AnalysisRunner(const AnalysisCatalog& catalog) noexcept : catalog_(catalog)
    {

    }

    AnalysisRun AnalysisRunner::run(const RunRecord& record, const std::vector<std::string>& names,
                                    const AnalysisOptions& options) const
    {
        AnalysisRun result;
        for (const std::string& name : names)
        {
            if (catalog_.knows(name))
                continue;
            result.error = "unknown analysis \"" + name + "\"; known:";
            for (const std::string& known : catalog_.names())
                result.error += " " + known;
            return result;
        }
        for (const std::string& name : names)
            result.tables.push_back(catalog_.make(name)->run(record, options));
        return result;
    }
} // namespace SushiProfiler
