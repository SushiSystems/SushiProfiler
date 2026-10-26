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
} // namespace

TEST(Small_SpanTable, AppendsSpansAndReadsThemBack)
{
    SpanTable table;
    const std::size_t row = table.append(span_at(100U));

    EXPECT_EQ(row, 0U);
    EXPECT_EQ(table.size(), 1U);
    EXPECT_EQ(table.span(0U).end_ns, 110U);
    EXPECT_EQ(table.span(0U).queued_ns, 99U);
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

    for (const AttributeColumn& column : table.attributes())
    {
        EXPECT_EQ(column.present.size(), 2U);
        EXPECT_EQ(column.present[1], 0U);
        EXPECT_EQ(column.integers.size(), 2U);
        EXPECT_EQ(column.reals.size(), 2U);
    }
    EXPECT_EQ(table.attribute(3U)->reals[0], 1.5);
    EXPECT_EQ(table.attribute(4U)->kind, AttributeKind::Name);
    EXPECT_EQ(table.attribute(4U)->integers[0], 9);
}

TEST(Small_SpanTable, EqualTablesCompareEqual)
{
    SpanTable first;
    SpanTable second;
    first.set_integer(first.append(span_at(5U)), 1U, 2);
    second.set_integer(second.append(span_at(5U)), 1U, 2);

    EXPECT_EQ(first, second);
    second.set_integer(0U, 1U, 3);
    EXPECT_FALSE(first == second);
}
