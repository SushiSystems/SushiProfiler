/**************************************************************************/
/* compare_records.cpp                                                    */
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
 * @file compare_records.cpp
 * @brief Checks the builds, groups periods by tags and compares each subject's median.
 * @author Mustafa Garip
 * @see modules/record/compare/README.md
 */

#include <SushiProfiler/compare/compare_records.hpp>

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include <SushiProfiler/analysis/subject_samples.hpp>
#include <SushiProfiler/statistics/sorted_samples.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief A median and its bootstrap interval.
        struct Estimate
        {
            double median = 0.0; /**< The median. */
            double low = 0.0;    /**< The interval's lower end. */
            double high = 0.0;   /**< The interval's upper end. */
        };

        /// @brief Returns the median of @p samples with its interval.
        Estimate estimate(const std::vector<double>& samples, const BootstrapSettings& bootstrap)
        {
            const Interval interval = bootstrap_median_interval(samples, bootstrap);
            return Estimate{SortedSamples(samples).median(), interval.low, interval.high};
        }

        /// @brief Returns @p numerator over @p denominator, or 0 over 0.
        double ratio(double numerator, double denominator) noexcept
        {
            return denominator != 0.0 ? numerator / denominator : 0.0;
        }

        /// @brief Returns the first build fact the two records disagree on, or an empty string.
        std::string build_difference(const RunRecord& baseline, const RunRecord& candidate)
        {
            std::map<std::string, std::pair<std::string, std::string>> facts;
            for (const Fact& fact : baseline.build)
                facts[fact.name].first = fact.value;
            for (const Fact& fact : candidate.build)
                facts[fact.name].second = fact.value;
            for (const auto& [name, values] : facts)
            {
                if (values.first == values.second)
                    continue;
                const auto shown = [](const std::string& value)
                {
                    return value.empty() ? std::string("nothing") : value;
                };
                return "\"" + name + "\": " + shown(values.first) + " against " +
                       shown(values.second);
            }
            return std::string();
        }

        /// @brief Returns @p period's tags as sorted `key=value` words joined by commas.
        std::string tag_label(const RunRecord& record, const PeriodRecord& period)
        {
            std::vector<std::string> words;
            for (const Tag& tag : period.tags)
            {
                words.push_back(std::string(record.name_of(tag.key)) + "=" +
                                std::string(record.name_of(tag.value)));
            }
            std::sort(words.begin(), words.end());
            std::string label;
            for (const std::string& word : words)
                label += (label.empty() ? "" : ",") + word;
            return label;
        }

        /// @brief Returns whether any active period of @p record carries a tag.
        bool carries_tags(const RunRecord& record) noexcept
        {
            return std::any_of(record.periods.begin(), record.periods.end(),
                               [](const PeriodRecord& period)
                               {
                                   return !period.warmup && !period.tags.empty();
                               });
        }

        /// @brief Returns the active periods of @p record, by tag label in first-seen order.
        std::vector<std::pair<std::string, std::vector<const PeriodRecord*>>> by_tags(
            const RunRecord& record, bool grouped)
        {
            std::vector<std::pair<std::string, std::vector<const PeriodRecord*>>> groups;
            for (const PeriodRecord& period : record.periods)
            {
                if (period.warmup)
                    continue;
                const std::string label = grouped ? tag_label(record, period) : std::string();
                auto found = std::find_if(groups.begin(), groups.end(),
                                          [&label](const auto& group)
                                          {
                                              return group.first == label;
                                          });
                if (found == groups.end())
                {
                    groups.emplace_back(label, std::vector<const PeriodRecord*>{});
                    found = groups.end() - 1;
                }
                found->second.push_back(&period);
            }
            return groups;
        }

        /// @brief Returns the word for a change of @p candidate against @p baseline, if any.
        std::string change_of(const Estimate& baseline, const Estimate& candidate,
                              const std::string& unit)
        {
            const bool time = unit == "ns";
            if (candidate.high < baseline.low)
                return time ? "faster" : "lower";
            if (candidate.low > baseline.high)
                return time ? "slower" : "higher";
            return std::string();
        }

        /// @brief Appends one row per subject both groups hold.
        void compare_group(Table& table, const std::string& label,
                           const std::vector<SubjectSamples>& baseline,
                           const std::vector<SubjectSamples>& candidate,
                           const BootstrapSettings& bootstrap)
        {
            for (const SubjectSamples& before : baseline)
            {
                const auto after = std::find_if(candidate.begin(), candidate.end(),
                                                [&before](const SubjectSamples& subject)
                                                {
                                                    return subject.subject == before.subject;
                                                });
                if (after == candidate.end())
                    continue;
                const Estimate base = estimate(before.samples, bootstrap);
                const Estimate next = estimate(after->samples, bootstrap);
                table.rows.push_back(Row{
                    Cell::text(label), Cell::text(before.subject), Cell::text(before.unit),
                    Cell::real(base.median), Cell::real(base.low), Cell::real(base.high),
                    Cell::real(next.median), Cell::real(next.low), Cell::real(next.high),
                    Cell::real(ratio(next.median, base.median)),
                    Cell::real(ratio(next.low, base.high)), Cell::real(ratio(next.high, base.low)),
                    Cell::text(change_of(base, next, before.unit))});
            }
        }
    } // namespace

    Comparison compare_records(const RunRecord& baseline, const RunRecord& candidate,
                               const CompareOptions& options)
    {
        Comparison comparison;
        const std::string difference = build_difference(baseline, candidate);
        if (!difference.empty() && !options.allow_build_mismatch)
        {
            comparison.error = "the builds differ at " + difference +
                               "; allow the mismatch to compare them anyway";
            return comparison;
        }
        Table& table = comparison.table;
        table.name = "compare";
        for (const char* const column : {"tags", "subject", "unit", "base", "base_low", "base_high",
                                         "median", "low", "high"})
            table.add_column(column);
        table.add_column("ratio", Unit::Ratio);
        table.add_column("ratio_low", Unit::Ratio);
        table.add_column("ratio_high", Unit::Ratio);
        table.add_column("change");
        if (!difference.empty())
            table.notes.push_back("The builds differ at " + difference + ".");
        const bool grouped = carries_tags(baseline) && carries_tags(candidate);
        const auto candidate_groups = by_tags(candidate, grouped);
        for (const auto& [label, periods] : by_tags(baseline, grouped))
        {
            const auto match = std::find_if(candidate_groups.begin(), candidate_groups.end(),
                                            [&label](const auto& group)
                                            {
                                                return group.first == label;
                                            });
            if (match == candidate_groups.end())
                continue;
            compare_group(table, label, subject_samples(baseline, periods),
                          subject_samples(candidate, match->second), options.bootstrap);
        }
        return comparison;
    }
} // namespace SushiProfiler
