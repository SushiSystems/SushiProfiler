/**************************************************************************/
/* test_span_sink.cpp                                                     */
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
 * @file test_span_sink.cpp
 * @brief Tests that the sink stores valid spans and attributes and refuses and counts the rest.
 * @author Mustafa Garip
 * @see tests/session/sources/README.md
 */

#include <SushiProfiler/sources/span_sink.hpp>

#include <gtest/gtest.h>

using SushiProfiler::NameID;
using SushiProfiler::NameTable;
using SushiProfiler::Span;
using SushiProfiler::SpanSink;
using SushiProfiler::SpanTable;

namespace
{
    /// @brief Holds a name table with `conv` and `bytes`, one track and a sink over a table.
    class Small_SpanSink : public ::testing::Test
    {
        protected:
            /// @brief Returns a span of `conv` on track 0 from 10 to 30 ns, queued at 5.
            Span conv_span() const
            {
                Span span;
                span.queued_ns = 5U;
                span.start_ns = 10U;
                span.end_ns = 30U;
                span.name = conv_;
                return span;
            }

            NameTable names_;                                  /**< The session's names. */
            NameID conv_ = names_.intern("conv");              /**< A span name. */
            NameID bytes_ = names_.intern("bytes");            /**< An attribute key. */
            std::vector<std::string> tracks_ = {"gpu.queue0"}; /**< The one track. */
            SpanTable table_;                                  /**< The period's spans. */
            SpanSink sink_{table_, names_, tracks_};           /**< The sink under test. */
    };
} // namespace

TEST_F(Small_SpanSink, StoresASpanAndItsAttributes)
{
    const auto row = sink_.append(conv_span());

    ASSERT_TRUE(row.has_value());
    EXPECT_TRUE(sink_.set_integer(*row, bytes_, 4096));
    EXPECT_TRUE(sink_.set_name(*row, conv_, conv_));
    ASSERT_EQ(table_.size(), 1U);
    EXPECT_EQ(table_.span(0U).end_ns, 30U);
    EXPECT_EQ(table_.attribute(bytes_)->integers, (std::vector<std::int64_t>{4096}));
    EXPECT_EQ(sink_.refused(), 0U);
}

TEST_F(Small_SpanSink, RefusesSpansTheRecordCannotHold)
{
    Span unknown_track = conv_span();
    unknown_track.track = 1U;
    Span unknown_name = conv_span();
    unknown_name.name = 9U;
    Span reversed = conv_span();
    reversed.end_ns = 9U;
    Span queued_late = conv_span();
    queued_late.queued_ns = 11U;

    EXPECT_FALSE(sink_.append(unknown_track).has_value());
    EXPECT_FALSE(sink_.append(unknown_name).has_value());
    EXPECT_FALSE(sink_.append(reversed).has_value());
    EXPECT_FALSE(sink_.append(queued_late).has_value());
    EXPECT_EQ(table_.size(), 0U);
    EXPECT_EQ(sink_.refused(), 4U);
}

TEST_F(Small_SpanSink, CountsLateSpansASourceDroppedApartFromRefusedOnes)
{
    sink_.drop_late(2U);
    sink_.drop_late(3U);

    EXPECT_EQ(sink_.late(), 5U);
    EXPECT_EQ(sink_.refused(), 0U);
    EXPECT_EQ(table_.size(), 0U);
}

TEST_F(Small_SpanSink, RefusesAttributesOnABadRowKeyValueOrKind)
{
    const std::size_t row = *sink_.append(conv_span());
    ASSERT_TRUE(sink_.set_integer(row, bytes_, 1));

    EXPECT_FALSE(sink_.set_integer(row + 1U, bytes_, 1));
    EXPECT_FALSE(sink_.set_integer(row, 0U, 1));
    EXPECT_FALSE(sink_.set_integer(row, 9U, 1));
    EXPECT_FALSE(sink_.set_real(row, bytes_, 1.5));
    EXPECT_FALSE(sink_.set_name(row, conv_, 9U));
    EXPECT_EQ(sink_.refused(), 5U);
    EXPECT_EQ(table_.attributes().size(), 1U);
}
