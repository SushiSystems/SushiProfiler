/**************************************************************************/
/* test_span_table.cpp                                                    */
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
 * @file test_span_table.cpp
 * @brief Tests that a span table keeps every column one entry per span.
 * @author Mustafa Garip
 * @see tests/record/record/README.md
 */

#include <SushiProfiler/record/span_table.hpp>

#include <gtest/gtest.h>

using SushiProfiler::AttributeColumn;
using SushiProfiler::AttributeKind;
using SushiProfiler::Span;
using SushiProfiler::SpanTable;

namespace
{
    /// @brief Returns a span that starts at @p start_ns and lasts 10 ns.
    Span span_at(std::uint64_t start_ns)
    {
        Span span;
        span.queued_ns = start_ns - 1U;
        span.start_ns = start_ns;
        span.end_ns = start_ns + 10U;
        span.track = 1U;
        span.name = 2U;
        return span;
    }

    /// @brief Returns a span whose every field differs from the default.
    Span full_span()
    {
        return Span{99U, 100U, 110U, 1U, 2U, 3U, 4U};
    }

    /// @brief A column of a span table and a change to a span that reaches it alone.
    struct ColumnChange
    {
        const char* column;
        void (*change)(Span&);
    };
} // namespace

namespace
{
    /// @brief Expects every vector of @p column to hold @p spans entries.
    void expect_aligned(const AttributeColumn& column, std::size_t spans)
    {
        EXPECT_EQ(column.present.size(), spans);
        EXPECT_EQ(column.integers.size(), spans);
        EXPECT_EQ(column.reals.size(), spans);
    }
} // namespace

TEST(Small_SpanTable, AppendsSpansAndReadsThemBack)
{
    SpanTable table;
    const std::size_t row = table.append(full_span());

    EXPECT_EQ(row, 0U);
    EXPECT_EQ(table.size(), 1U);
    const Span read = table.span(0U);
    EXPECT_EQ(read.queued_ns, 99U);
    EXPECT_EQ(read.start_ns, 100U);
    EXPECT_EQ(read.end_ns, 110U);
    EXPECT_EQ(read.track, 1U);
    EXPECT_EQ(read.name, 2U);
    EXPECT_EQ(read.submit_thread, 3U);
    EXPECT_EQ(read.uncertainty_ns, 4U);
}

TEST(Small_SpanTable, AColumnAddedLateMarksEarlierSpansAbsent)
{
    SpanTable table;
    table.append(span_at(100U));
    const std::size_t second = table.append(span_at(200U));
    table.set_integer(second, 7U, 42);

    const AttributeColumn* const column = table.attribute(7U);
    ASSERT_NE(column, nullptr);
    EXPECT_EQ(column->kind, AttributeKind::Integer);
    EXPECT_EQ(column->present, (std::vector<std::uint8_t>{0U, 1U}));
    EXPECT_EQ(column->integers[1], 42);
}

TEST(Small_SpanTable, ASpanAddedLateIsAbsentFromEveryColumn)
{
    SpanTable table;
    const std::size_t first = table.append(span_at(100U));
    table.set_real(first, 3U, 1.5);
    table.set_name(first, 4U, 9U);
    table.append(span_at(200U));

    const AttributeColumn* const real = table.attribute(3U);
    const AttributeColumn* const name = table.attribute(4U);
    ASSERT_NE(real, nullptr);
    ASSERT_NE(name, nullptr);
    expect_aligned(*real, 2U);
    expect_aligned(*name, 2U);
    EXPECT_EQ(real->present, (std::vector<std::uint8_t>{1U, 0U}));
    EXPECT_EQ(name->present, (std::vector<std::uint8_t>{1U, 0U}));
    EXPECT_EQ(real->reals[0], 1.5);
    EXPECT_EQ(name->kind, AttributeKind::Name);
    EXPECT_EQ(name->integers[0], 9);
}

TEST(Small_SpanTable, AColumnReadFromAFileIsAddedAsItIs)
{
    SpanTable table;
    table.append(span_at(100U));
    table.append(span_at(200U));
    AttributeColumn column;
    column.key = 5U;
    column.kind = AttributeKind::Integer;
    column.integers = {7, 0};
    column.reals = {0.0, 0.0};
    column.present = {1U, 0U};

    table.add_column(column);

    ASSERT_NE(table.attribute(5U), nullptr);
    EXPECT_EQ(*table.attribute(5U), column);
}

TEST(Small_SpanTable, EqualTablesCompareEqual)
{
    SpanTable first;
    SpanTable second;
    first.set_integer(first.append(full_span()), 1U, 2);
    second.set_integer(second.append(full_span()), 1U, 2);

    EXPECT_EQ(first, second);
}

TEST(Small_SpanTable, AnAttributeValueBreaksEquality)
{
    SpanTable first;
    SpanTable second;
    first.set_integer(first.append(full_span()), 1U, 2);
    second.set_integer(second.append(full_span()), 1U, 3);

    EXPECT_FALSE(first == second);
}

class Small_SpanTableColumn : public ::testing::TestWithParam<ColumnChange>
{
};

TEST_P(Small_SpanTableColumn, EqualityReachesTheColumn)
{
    SpanTable first;
    SpanTable second;
    first.append(full_span());
    Span changed = full_span();
    GetParam().change(changed);
    second.append(changed);

    EXPECT_FALSE(first == second) << GetParam().column;
}

INSTANTIATE_TEST_SUITE_P(
    Small_Columns, Small_SpanTableColumn,
    ::testing::Values(
        ColumnChange{"queued_ns", [](Span& span) { span.queued_ns = 0U; }},
        ColumnChange{"start_ns", [](Span& span) { span.start_ns = 0U; }},
        ColumnChange{"end_ns", [](Span& span) { span.end_ns = 0U; }},
        ColumnChange{"track", [](Span& span) { span.track = 9U; }},
        ColumnChange{"name", [](Span& span) { span.name = 9U; }},
        ColumnChange{"submit_thread", [](Span& span) { span.submit_thread = 9U; }},
        ColumnChange{"uncertainty_ns", [](Span& span) { span.uncertainty_ns = 9U; }}));
