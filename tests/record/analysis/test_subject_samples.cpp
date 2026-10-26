/**************************************************************************/
/* test_subject_samples.cpp                                               */
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
 * @file test_subject_samples.cpp
 * @brief Tests every subject the shared records hold, in order, with its samples.
 * @author Mustafa Garip
 * @see tests/record/analysis/README.md
 */

#include <SushiProfiler/analysis/subject_samples.hpp>

#include <gtest/gtest.h>

#include "analysis_record.hpp"

using SushiProfiler::SubjectSamples;

namespace
{
    /// @brief Expects @p actual to name @p subject in @p unit with @p samples.
    void expect_subject(const SubjectSamples& actual, const char* subject, const char* unit,
                        const std::vector<double>& samples)
    {
        EXPECT_EQ(actual.subject, subject);
        EXPECT_EQ(actual.unit, unit) << subject;
        EXPECT_EQ(actual.samples, samples) << subject;
    }
} // namespace

TEST(Small_SubjectSamples, ListsEveryHostSubjectInOrder)
{
    const std::vector<SubjectSamples> subjects =
        SushiProfiler::subject_samples(SushiProfilerTests::analysis_record());

    ASSERT_EQ(subjects.size(), 11U);
    expect_subject(subjects[0], "timeline:step", "ns", {100.0, 120.0, 110.0});
    expect_subject(subjects[1], "group:step/Data", "ns", {30.0, 20.0, 30.0});
    expect_subject(subjects[2], "group:step/Model", "ns", {60.0, 80.0, 70.0});
    expect_subject(subjects[3], "group:step/(uncovered)", "ns", {10.0, 20.0, 10.0});
    expect_subject(subjects[4], "zone:forward", "ns", {60.0, 80.0, 70.0});
    expect_subject(subjects[5], "zone:conv", "ns", {20.0, 30.0});
    expect_subject(subjects[6], "zone:load", "ns", {30.0, 20.0, 30.0});
    expect_subject(subjects[7], "counter:bytes", "B", {150.0, 200.0});
    expect_subject(subjects[8], "counter:peak", "B", {9.0, 4.0, 7.0});
    expect_subject(subjects[9], "value:loss", "", {1.0, 3.0, 2.0});
    expect_subject(subjects[10], "flow:item", "ns", {100.0, 90.0});
}

TEST(Small_SubjectSamples, ListsSpansByName)
{
    const std::vector<SubjectSamples> subjects =
        SushiProfiler::subject_samples(SushiProfilerTests::span_record());

    ASSERT_EQ(subjects.size(), 4U);
    expect_subject(subjects[0], "timeline:step", "ns", {1000.0, 1000.0});
    expect_subject(subjects[1], "group:step/(uncovered)", "ns", {1000.0, 1000.0});
    expect_subject(subjects[2], "span:copy", "ns", {5.0, 8.0});
    expect_subject(subjects[3], "span:gemm", "ns", {300.0, 300.0, 600.0});
}
