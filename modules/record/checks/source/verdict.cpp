/**************************************************************************/
/* verdict.cpp                                                            */
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
 * @file verdict.cpp
 * @brief Takes each check's statistic from the subject's samples and tests it against the bounds.
 * @author Mustafa Garip
 * @see modules/record/checks/README.md
 */

#include <SushiProfiler/checks/verdict.hpp>

#include <algorithm>
#include <cstdio>

#include <SushiProfiler/analysis/subject_samples.hpp>
#include <SushiProfiler/statistics/sorted_samples.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns @p statistic of @p samples.
        double statistic_of(const SortedSamples& samples, Statistic statistic) noexcept
        {
            switch (statistic)
            {
                case Statistic::Median: return samples.median();
                case Statistic::P50: return samples.percentile(50.0);
                case Statistic::P90: return samples.percentile(90.0);
                case Statistic::P95: return samples.percentile(95.0);
                case Statistic::P99: return samples.percentile(99.0);
                case Statistic::Min: return samples.minimum();
                case Statistic::Max: return samples.maximum();
            }
            return samples.median();
        }

        /// @brief Returns @p value with six significant digits and @p unit.
        std::string shown(double value, const std::string& unit)
        {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%.6g", value);
            return unit.empty() ? std::string(buffer) : std::string(buffer) + " " + unit;
        }
    } // namespace

    std::vector<Verdict> check(const RunRecord& record, const std::vector<Check>& checks)
    {
        const std::vector<SubjectSamples> subjects = subject_samples(record);
        std::vector<Verdict> verdicts;
        for (const Check& current : checks)
        {
            Verdict verdict;
            verdict.check = current;
            const auto found = std::find_if(subjects.begin(), subjects.end(),
                                            [&current](const SubjectSamples& subject)
                                            {
                                                return subject.subject == current.subject;
                                            });
            if (found == subjects.end())
            {
                verdict.reason = "the record has no samples of " + current.subject;
                verdicts.push_back(std::move(verdict));
                continue;
            }
            verdict.value = statistic_of(SortedSamples(found->samples), current.statistic);
            const std::string name(statistic_word(current.statistic));
            if (current.at_most && verdict.value > *current.at_most)
            {
                verdict.reason = name + " is " + shown(verdict.value, found->unit) +
                                 ", above at most " + shown(*current.at_most, found->unit);
            }
            else if (current.at_least && verdict.value < *current.at_least)
            {
                verdict.reason = name + " is " + shown(verdict.value, found->unit) +
                                 ", below at least " + shown(*current.at_least, found->unit);
            }
            verdict.passed = verdict.reason.empty();
            verdicts.push_back(std::move(verdict));
        }
        return verdicts;
    }

    bool all_passed(const std::vector<Verdict>& verdicts) noexcept
    {
        return std::all_of(verdicts.begin(), verdicts.end(), [](const Verdict& verdict)
                           {
                               return verdict.passed;
                           });
    }
} // namespace SushiProfiler
