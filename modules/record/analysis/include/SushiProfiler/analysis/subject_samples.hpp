/**************************************************************************/
/* subject_samples.hpp                                                    */
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
 * @file subject_samples.hpp
 * @brief Declares the samples of every subject in a record, under the names checks use.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#pragma once

#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/version/export.hpp>

#include <string>
#include <vector>

namespace SushiProfiler
{
    /// @brief One subject and its samples, in period order.
    struct SubjectSamples
    {
        std::string subject;         /**< `<kind>:<name>`, such as `timeline:step`. */
        std::string unit;            /**< `ns` for times, else the site's unit. */
        std::vector<double> samples; /**< The samples. */
    };

    /**
     * @brief Returns the subjects of @p periods that have samples, in a fixed order.
     *
     * `timeline:<t>` period times; `group:<t>/<group>` self time per period on the marking
     * thread, with `(uncovered)` last; `zone:<site>` inclusive time per period that ran it;
     * `counter:<site>` the fold per period; `value:<site>` every sample; `span:<name>` every span
     * length; `flow:<site>` each item's begin-to-end latency.
     */
    [[nodiscard]] SUSHIPROFILER_API std::vector<SubjectSamples> subject_samples(
        const RunRecord& record, const std::vector<const PeriodRecord*>& periods);

    /// @brief Returns the subjects of every period of @p record that is not warm-up.
    [[nodiscard]] SUSHIPROFILER_API std::vector<SubjectSamples> subject_samples(
        const RunRecord& record);
} // namespace SushiProfiler
