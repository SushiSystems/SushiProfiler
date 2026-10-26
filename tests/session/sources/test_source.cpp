/**************************************************************************/
/* test_source.cpp                                                        */
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
 * @file test_source.cpp
 * @brief Tests a source through its interface: begin adds a track, collect adds a period's spans.
 * @author Mustafa Garip
 * @see tests/session/sources/README.md
 */

#include <SushiProfiler/sources/source.hpp>

#include <gtest/gtest.h>

using SushiProfiler::NameID;
using SushiProfiler::NameTable;
using SushiProfiler::Source;
using SushiProfiler::SourceContext;
using SushiProfiler::SourceStatus;
using SushiProfiler::Span;
using SushiProfiler::SpanSink;
using SushiProfiler::SpanTable;
using SushiProfiler::TrackID;

namespace
{
    /// @brief A source with one span per period, 100 ns apart from steady time 1100.
    class QueueSource final : public Source
    {
        public:
            /// @brief Builds a source that refuses to start when @p available is false.
            explicit QueueSource(bool available) noexcept : available_(available)
            {

            }

            /// @brief Returns `test.queue`.
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "test.queue";
            }

            /// @brief Adds the `device` track and the `kernel` name, unless unavailable.
            [[nodiscard]] SourceStatus begin(SourceContext& context) override
            {
                if (!available_)
                    return SourceStatus{"no device"};
                track_ = context.add_track("device");
                kernel_ = context.intern("kernel");
                start_ns_ = context.to_record_ns(1'100U);
                return SourceStatus{};
            }

            /// @brief Adds the 40 ns span of @p period.
            void collect(std::uint64_t period, SpanSink& sink) override
            {
                Span span;
                span.queued_ns = start_ns_ + period * 100U;
                span.start_ns = span.queued_ns;
                span.end_ns = span.start_ns + 40U;
                span.track = track_;
                span.name = kernel_;
                static_cast<void>(sink.append(span));
            }

            /// @brief Notes that the source ended.
            void end() override
            {
                ended_ = true;
            }

            /// @brief Returns whether end() ran.
            [[nodiscard]] bool ended() const noexcept
            {
                return ended_;
            }

        private:
            bool available_;              /**< Whether begin() succeeds. */
            TrackID track_ = 0U;          /**< The device track. */
            NameID kernel_ = 0U;          /**< The span name. */
            std::uint64_t start_ns_ = 0U; /**< The first span's start on the record's clock. */
            bool ended_ = false;          /**< Whether end() ran. */
    };
} // namespace

TEST(Small_Source, BeginsCollectsEachPeriodAndEnds)
{
    NameTable names;
    std::vector<std::string> tracks;
    SourceContext context(names, tracks, 1'000U);
    QueueSource source(true);
    Source& as_source = source;

    ASSERT_TRUE(as_source.begin(context).ok());
    SpanTable period_two;
    SpanSink sink(period_two, names, tracks);
    as_source.collect(2U, sink);
    as_source.end();

    EXPECT_EQ(as_source.name(), "test.queue");
    EXPECT_EQ(tracks, std::vector<std::string>{"device"});
    ASSERT_EQ(period_two.size(), 1U);
    EXPECT_EQ(period_two.span(0U).start_ns, 300U);
    EXPECT_TRUE(source.ended());
}

TEST(Small_Source, ASourceThatCannotStartSaysWhy)
{
    NameTable names;
    std::vector<std::string> tracks;
    SourceContext context(names, tracks, 0U);
    QueueSource source(false);

    const SourceStatus status = source.begin(context);

    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.error, "no device");
    EXPECT_TRUE(tracks.empty());
}
