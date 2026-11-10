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
 * @file test_span_sink.cpp
 * @brief Tests that the sink stores valid spans and attributes and refuses and counts the rest.
 * @author Mustafa Garip
 * @see tests/session/sources/README.md
 */

#include <SushiProfiler/sources/span_sink.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

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

TEST_F(Small_SpanSink, StoresASpan)
{
    const auto row = sink_.append(conv_span());

    ASSERT_TRUE(row.has_value());
    ASSERT_EQ(table_.size(), 1U);
    EXPECT_EQ(table_.span(0U).end_ns, 30U);
    EXPECT_EQ(sink_.refused(), 0U);
}

TEST_F(Small_SpanSink, StoresAnIntegerAttributeOfASpan)
{
    const std::size_t row = *sink_.append(conv_span());

    EXPECT_TRUE(sink_.set_integer(row, bytes_, 4096));

    EXPECT_EQ(table_.attribute(bytes_)->integers, (std::vector<std::int64_t>{4096}));
    EXPECT_EQ(sink_.refused(), 0U);
}

TEST_F(Small_SpanSink, StoresARealAttributeOfASpan)
{
    const std::size_t row = *sink_.append(conv_span());

    EXPECT_TRUE(sink_.set_real(row, bytes_, 1.5));

    EXPECT_EQ(table_.attribute(bytes_)->reals, (std::vector<double>{1.5}));
    EXPECT_EQ(sink_.refused(), 0U);
}

TEST_F(Small_SpanSink, StoresANameAttributeOfASpan)
{
    const std::size_t row = *sink_.append(conv_span());

    EXPECT_TRUE(sink_.set_name(row, bytes_, conv_));

    EXPECT_EQ(table_.attribute(bytes_)->integers,
              (std::vector<std::int64_t>{static_cast<std::int64_t>(conv_)}));
    EXPECT_EQ(sink_.refused(), 0U);
}

namespace
{
    /// @brief One change that makes a span the record cannot hold.
    struct SpanRefusalRow
    {
        void (*change)(Span&);
    };

    /// @brief One attempt to set an attribute that the sink must refuse.
    struct AttributeRefusalRow
    {
        bool (*attempt)(SpanSink& sink, std::size_t row, NameID key, NameID name);
    };

    /// @brief Points the span at a track the sink lacks.
    void unknown_track(Span& span)
    {
        span.track = 1U;
    }

    /// @brief Gives the span a name the table lacks.
    void unknown_name(Span& span)
    {
        span.name = 9U;
    }

    /// @brief Ends the span before it starts.
    void reversed(Span& span)
    {
        span.end_ns = 9U;
    }

    /// @brief Queues the span after it started.
    void queued_late(Span& span)
    {
        span.queued_ns = 11U;
    }

    /// @brief Sets an integer on a row past the table.
    bool integer_on_a_missing_row(SpanSink& sink, std::size_t row, NameID key, NameID)
    {
        return sink.set_integer(row + 1U, key, 1);
    }

    /// @brief Sets an integer under key 0.
    bool integer_under_the_empty_key(SpanSink& sink, std::size_t row, NameID, NameID)
    {
        return sink.set_integer(row, 0U, 1);
    }

    /// @brief Sets an integer under a key the table lacks.
    bool integer_under_an_unknown_key(SpanSink& sink, std::size_t row, NameID, NameID)
    {
        return sink.set_integer(row, 9U, 1);
    }

    /// @brief Sets a real under a key that holds integers.
    bool real_under_an_integer_key(SpanSink& sink, std::size_t row, NameID key, NameID)
    {
        return sink.set_real(row, key, 1.5);
    }

    /// @brief Sets a name value the table lacks.
    bool unknown_name_value(SpanSink& sink, std::size_t row, NameID, NameID name)
    {
        return sink.set_name(row, name, 9U);
    }
} // namespace

class Small_SpanSinkRefusesSpans : public Small_SpanSink,
                                   public ::testing::WithParamInterface<SpanRefusalRow>
{
};

TEST_P(Small_SpanSinkRefusesSpans, ASpanTheRecordCannotHoldIsRefusedAndCounted)
{
    Span span = conv_span();
    GetParam().change(span);

    EXPECT_FALSE(sink_.append(span).has_value());

    EXPECT_EQ(table_.size(), 0U);
    EXPECT_EQ(sink_.refused(), 1U);
}

INSTANTIATE_TEST_SUITE_P(Small_Spans, Small_SpanSinkRefusesSpans,
                         ::testing::Values(SpanRefusalRow{unknown_track},
                                           SpanRefusalRow{unknown_name}, SpanRefusalRow{reversed},
                                           SpanRefusalRow{queued_late}));

class Small_SpanSinkRefusesAttributes : public Small_SpanSink,
                                        public ::testing::WithParamInterface<AttributeRefusalRow>
{
};

TEST_P(Small_SpanSinkRefusesAttributes, AnAttributeOnABadRowKeyValueOrKindIsRefusedAndCounted)
{
    const std::size_t row = *sink_.append(conv_span());
    ASSERT_TRUE(sink_.set_integer(row, bytes_, 1));

    EXPECT_FALSE(GetParam().attempt(sink_, row, bytes_, conv_));

    EXPECT_EQ(sink_.refused(), 1U);
    EXPECT_EQ(table_.attributes().size(), 1U);
}

INSTANTIATE_TEST_SUITE_P(
    Small_Attributes, Small_SpanSinkRefusesAttributes,
    ::testing::Values(AttributeRefusalRow{integer_on_a_missing_row},
                      AttributeRefusalRow{integer_under_the_empty_key},
                      AttributeRefusalRow{integer_under_an_unknown_key},
                      AttributeRefusalRow{real_under_an_integer_key},
                      AttributeRefusalRow{unknown_name_value}));

TEST_F(Small_SpanSink, CountsLateSpansASourceDroppedApartFromRefusedOnes)
{
    sink_.drop_late(2U);
    sink_.drop_late(3U);

    EXPECT_EQ(sink_.late(), 5U);
    EXPECT_EQ(sink_.refused(), 0U);
    EXPECT_EQ(table_.size(), 0U);
}

TEST_F(Small_SpanSink, SetsAFactOnThePeriod)
{
    std::vector<SushiProfiler::Fact> facts;
    SpanSink sink(table_, facts, names_, tracks_);

    EXPECT_TRUE(sink.set_fact("execution.layer", "graph"));
    EXPECT_TRUE(sink.set_fact("queue.depth", "3"));

    ASSERT_EQ(facts.size(), 2U);
    EXPECT_EQ(facts[0].name, "execution.layer");
    EXPECT_EQ(facts[0].value, "graph");
    EXPECT_EQ(sink.refused(), 0U);
}

TEST_F(Small_SpanSink, ReplacesAFactOfTheSameName)
{
    std::vector<SushiProfiler::Fact> facts;
    SpanSink sink(table_, facts, names_, tracks_);
    static_cast<void>(sink.set_fact("queue.depth", "3"));

    EXPECT_TRUE(sink.set_fact("queue.depth", "4"));

    ASSERT_EQ(facts.size(), 1U);
    EXPECT_EQ(facts[0].value, "4");
}

TEST_F(Small_SpanSink, RefusesAnEmptyFactName)
{
    std::vector<SushiProfiler::Fact> facts;
    SpanSink sink(table_, facts, names_, tracks_);

    EXPECT_FALSE(sink.set_fact("", "x"));

    EXPECT_TRUE(facts.empty());
    EXPECT_EQ(sink.refused(), 1U);
}

TEST_F(Small_SpanSink, RefusesAFactOnASinkBuiltWithoutFacts)
{
    EXPECT_FALSE(sink_.set_fact("queue.depth", "3"));

    EXPECT_EQ(sink_.refused(), 1U);
}
