/**************************************************************************/
/* test_compare_records.cpp                                               */
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
 * @file test_compare_records.cpp
 * @brief Tests ratios, the overlap rule, build checks and grouping by tags.
 * @author Mustafa Garip
 * @see tests/record/compare/README.md
 */

#include <SushiProfiler/compare/compare_records.hpp>

#include <gtest/gtest.h>

#include <initializer_list>

using SushiProfiler::Cell;
using SushiProfiler::Comparison;
using SushiProfiler::CompareOptions;
using SushiProfiler::PeriodRecord;
using SushiProfiler::Row;
using SushiProfiler::RunRecord;
using SushiProfiler::NameID;
using SushiProfiler::Tag;

namespace
{
    /// @brief The names every test record shares: timeline `step`, tag `layer` and its values.
    const std::vector<std::string> K_NAMES = {"", "step", "layer", "fused", "plain"};

    /// @brief Returns a record of timeline `step` with periods of @p lengths tagged by @p tag.
    RunRecord record_of(std::initializer_list<std::uint64_t> lengths, NameID tag = 0U)
    {
        RunRecord record;
        record.names = K_NAMES;
        for (const std::uint64_t length : lengths)
        {
            PeriodRecord period;
            period.timeline = 1U;
            period.end_ns = length;
            if (tag != 0U)
                period.tags.push_back(Tag{2U, tag});
            record.periods.push_back(period);
        }
        return record;
    }

    /// @brief Appends @p other's periods to @p record.
    void append(RunRecord& record, const RunRecord& other)
    {
        record.periods.insert(record.periods.end(), other.periods.begin(), other.periods.end());
    }

    /// @brief Returns the row of @p subject in @p comparison, or null.
    const Row* row_of(const Comparison& comparison, const std::string& tags,
                      const std::string& subject)
    {
        for (const Row& row : comparison.table.rows)
        {
            if (*row[0].as_text() == tags && *row[1].as_text() == subject)
                return &row;
        }
        return nullptr;
    }
} // namespace

TEST(Small_CompareRecords, NamesAChangeWhenTheIntervalsAreApart)
{
    const Comparison comparison = SushiProfiler::compare_records(
        record_of({100U, 100U, 100U, 100U}), record_of({50U, 50U, 50U, 50U}));

    ASSERT_TRUE(comparison.ok()) << comparison.error;
    const Row* const row = row_of(comparison, "", "timeline:step");
    ASSERT_NE(row, nullptr);
    const Row expected = {Cell::text(""),       Cell::text("timeline:step"), Cell::text("ns"),
                          Cell::real(100.0),    Cell::real(100.0),           Cell::real(100.0),
                          Cell::real(50.0),     Cell::real(50.0),            Cell::real(50.0),
                          Cell::real(0.5),      Cell::real(0.5),             Cell::real(0.5),
                          Cell::text("faster")};
    EXPECT_EQ(*row, expected);
}

TEST(Small_CompareRecords, OverlappingIntervalsAreNoChange)
{
    const Comparison comparison = SushiProfiler::compare_records(
        record_of({100U, 110U, 120U}), record_of({105U, 115U, 125U}));

    const Row* const row = row_of(comparison, "", "timeline:step");
    ASSERT_NE(row, nullptr);
    EXPECT_EQ((*row)[12], Cell::text(""));
    EXPECT_NEAR(*(*row)[9].as_real(), 115.0 / 110.0, 1e-12);
}

TEST(Small_CompareRecords, ASlowerCandidateIsNamed)
{
    const Comparison comparison =
        SushiProfiler::compare_records(record_of({50U, 50U, 50U}), record_of({80U, 80U, 80U}));

    EXPECT_EQ((*row_of(comparison, "", "timeline:step"))[12], Cell::text("slower"));
}

TEST(Small_CompareRecords, RefusesDifferingBuildsUnlessAllowed)
{
    RunRecord baseline = record_of({100U});
    RunRecord candidate = record_of({100U});
    baseline.build = {{"type", "Release"}};
    candidate.build = {{"type", "Debug"}};

    const Comparison refused = SushiProfiler::compare_records(baseline, candidate);
    EXPECT_FALSE(refused.ok());
    EXPECT_EQ(refused.error, "the builds differ at \"type\": Release against Debug; allow the "
                             "mismatch to compare them anyway");

    CompareOptions options;
    options.allow_build_mismatch = true;
    const Comparison allowed = SushiProfiler::compare_records(baseline, candidate, options);
    ASSERT_TRUE(allowed.ok());
    EXPECT_EQ(allowed.table.notes,
              std::vector<std::string>{"The builds differ at \"type\": Release against Debug."});
}

TEST(Small_CompareRecords, GroupsRowsByTagsWhenBothRecordsCarryThem)
{
    RunRecord baseline = record_of({100U, 100U}, 3U);
    append(baseline, record_of({200U, 200U}, 4U));
    RunRecord candidate = record_of({50U, 50U}, 3U);
    append(candidate, record_of({200U, 200U}, 4U));
    const Comparison comparison = SushiProfiler::compare_records(baseline, candidate);

    const Row* const fused = row_of(comparison, "layer=fused", "timeline:step");
    const Row* const plain = row_of(comparison, "layer=plain", "timeline:step");
    ASSERT_NE(fused, nullptr);
    ASSERT_NE(plain, nullptr);
    EXPECT_EQ((*fused)[12], Cell::text("faster"));
    EXPECT_EQ((*plain)[12], Cell::text(""));
}

TEST(Small_CompareRecords, ASubjectInOneRecordOnlyHasNoRow)
{
    RunRecord candidate = record_of({100U});
    candidate.names.push_back("other");
    candidate.periods[0].timeline = 5U;
    const Comparison comparison = SushiProfiler::compare_records(record_of({100U}), candidate);

    EXPECT_EQ(row_of(comparison, "", "timeline:step"), nullptr);
    EXPECT_EQ(row_of(comparison, "", "timeline:other"), nullptr);
}
