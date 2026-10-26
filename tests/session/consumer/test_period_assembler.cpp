/**************************************************************************/
/* test_period_assembler.cpp                                              */
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
 * @file test_period_assembler.cpp
 * @brief Tests placing drained events in periods: marks, the window, linger, late and outside.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/period_assembler.hpp>

#include <gtest/gtest.h>

#include <string>

using SushiProfiler::AssemblySettings;
using SushiProfiler::CounterOp;
using SushiProfiler::DrainBatch;
using SushiProfiler::EventKind;
using SushiProfiler::NamedStreamWriter;
using SushiProfiler::NameID;
using SushiProfiler::NameTable;
using SushiProfiler::PeriodAssembler;
using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::Site;
using SushiProfiler::SiteHandle;
using SushiProfiler::SiteID;
using SushiProfiler::SiteRegistry;
using SushiProfiler::SiteRole;
using SushiProfiler::Source;
using SushiProfiler::SourceContext;
using SushiProfiler::SourceStatus;
using SushiProfiler::Span;
using SushiProfiler::SpanSink;
using SushiProfiler::StreamWriter;
using SushiProfiler::ThreadDrain;
using SushiProfiler::TraceEvent;
using SushiProfiler::Window;
using SushiProfiler::WindowSchedule;
using SushiProfiler::WriteStatus;
using SushiProfiler::ZoneInstance;

namespace
{
    /// @brief The zone site every test records.
    constexpr Site K_CONV = {"conv", "Model", "test_period_assembler.cpp", 1U, 1U,
                             SiteRole::Zone, CounterOp::Add, nullptr};

    /// @brief Returns a zone of site 1 from @p begin to @p end, in steady ticks.
    TraceEvent zone(std::uint64_t begin, std::uint64_t end)
    {
        return TraceEvent{begin, end, SushiProfiler::pack_site(EventKind::Zone, 1U), 0U};
    }

    /// @brief Returns an event of @p kind at @p time with second field @p end and name @p name.
    TraceEvent event_of(EventKind kind, std::uint64_t time, std::uint64_t end, NameID name = 0U)
    {
        return TraceEvent{time, end, SushiProfiler::pack_site(kind, 0U), name};
    }

    /// @brief Returns a drain at steady time @p now holding @p threads.
    DrainBatch batch_at(std::uint64_t now, std::vector<ThreadDrain> threads)
    {
        DrainBatch batch;
        batch.point = {now, now};
        batch.threads = std::move(threads);
        return batch;
    }

    /// @brief Returns thread @p index's drain of @p events.
    ThreadDrain drain_of(std::uint32_t index, std::vector<TraceEvent> events)
    {
        ThreadDrain drain;
        drain.thread_index = index;
        drain.events = std::move(events);
        return drain;
    }

    /// @brief A source that hands over one span per period on its track.
    class StepSource final : public Source
    {
        public:
            /// @brief Returns `step`.
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "step";
            }

            /// @brief Adds track `device` and the span name `kernel`.
            [[nodiscard]] SourceStatus begin(SourceContext& context) override
            {
                track_ = context.add_track("device");
                kernel_ = context.intern("kernel");
                return SourceStatus{};
            }

            /// @brief Adds a 5 ns span 10 ns after the period's index times 100.
            void collect(std::uint64_t period, SpanSink& sink) override
            {
                Span span;
                span.start_ns = period * 100U + 110U;
                span.queued_ns = span.start_ns;
                span.end_ns = span.start_ns + 5U;
                span.track = track_;
                span.name = kernel_;
                static_cast<void>(sink.append(span));
                Span stray = span;
                stray.track = 9U;
                static_cast<void>(sink.append(stray));
            }

            /// @brief Ends.
            void end() override
            {

            }

        private:
            std::uint32_t track_ = 0U; /**< The device track. */
            NameID kernel_ = 0U;       /**< The span name. */
    };

    /// @brief A stream writer that notes each period's index and fails when told to.
    class NotingWriter final : public StreamWriter
    {
        public:
            /// @brief Starts.
            [[nodiscard]] WriteStatus begin(const RunRecord& record) override
            {
                static_cast<void>(record);
                return WriteStatus::Written;
            }

            /// @brief Notes the period and its runs, or fails.
            [[nodiscard]] WriteStatus write_period(
                const RunRecord& record, const PeriodRecord& period,
                const std::vector<ZoneInstance>& instances) override
            {
                static_cast<void>(record);
                periods += std::to_string(period.index) + ":" + std::to_string(instances.size()) +
                           " ";
                return fails ? WriteStatus::StreamFailed : WriteStatus::Written;
            }

            /// @brief Ends.
            [[nodiscard]] WriteStatus end(const RunRecord& record) override
            {
                static_cast<void>(record);
                return WriteStatus::Written;
            }

            std::string periods; /**< The periods written so far. */
            bool fails = false;  /**< Whether writes fail. */
    };

    /// @brief Holds the tables, the window and the record an assembler fills.
    class Small_PeriodAssembler : public ::testing::Test
    {
        protected:
            /// @brief Registers site 1 and the timeline name `step`.
            void SetUp() override
            {
                ASSERT_EQ(sites_.id_of(conv_), 1U);
                step_ = names_.intern("step");
            }

            /// @brief Returns settings that follow `step` with @p schedule and @p linger_ns.
            AssemblySettings follow(const WindowSchedule& schedule, std::uint64_t linger_ns)
            {
                window_ = std::make_unique<Window>(schedule);
                AssemblySettings settings;
                settings.timeline = step_;
                settings.window = window_.get();
                settings.linger_ns = linger_ns;
                return settings;
            }

            /// @brief Returns a mark of `step` opening period @p index at @p time.
            TraceEvent mark(std::uint64_t index, std::uint64_t time) const
            {
                return event_of(EventKind::PeriodMark, time, index, step_);
            }

            NameTable names_;                   /**< The session's names. */
            SiteRegistry sites_;                /**< The session's sites. */
            SiteHandle conv_{&K_CONV};          /**< Site 1's handle. */
            NameID step_ = 0U;                  /**< The followed timeline. */
            std::unique_ptr<Window> window_;    /**< The window of follow(). */
            RunRecord record_;                  /**< The record filled. */
    };

    /// @brief Returns one warm-up and two active periods, once.
    WindowSchedule warm_one_active_two()
    {
        WindowSchedule schedule;
        schedule.warmup = 1U;
        schedule.active = 2U;
        return schedule;
    }
} // namespace

TEST_F(Small_PeriodAssembler, RecordsARunWithoutATimelineAsOnePeriod)
{
    AssemblySettings settings;
    settings.run_timeline = step_;
    settings.linger_ns = 0U;
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, {});

    assembler.add(batch_at(500U, {drain_of(0U, {zone(10U, 20U), zone(30U, 40U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 1U);
    const PeriodRecord& period = record_.periods[0];
    EXPECT_EQ(period.timeline, step_);
    EXPECT_EQ(period.begin_ns, 0U);
    EXPECT_EQ(period.end_ns, 500U);
    ASSERT_EQ(period.zones.size(), 1U);
    EXPECT_EQ(period.zones[0].nodes.size(), 1U);
    EXPECT_EQ(period.zones[0].nodes[0].calls, 2U);
    EXPECT_EQ(record_.names, (std::vector<std::string>{"", "step"}));
    ASSERT_EQ(record_.sites.size(), 2U);
    EXPECT_EQ(record_.sites[1].group, "Model");
}

TEST_F(Small_PeriodAssembler, KeepsTheWindowsPeriodsAndCountsTheRestOutside)
{
    PeriodAssembler assembler(follow(warm_one_active_two(), 0U), record_, names_, sites_, {},
                              {});

    assembler.add(batch_at(
        1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U), mark(3U, 400U)}),
                 drain_of(1U, {zone(150U, 160U), zone(250U, 260U), zone(350U, 390U),
                               zone(450U, 460U), zone(50U, 60U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 3U);
    EXPECT_TRUE(record_.periods[0].warmup);
    EXPECT_FALSE(record_.periods[1].warmup);
    EXPECT_EQ(record_.periods[2].begin_ns, 300U);
    EXPECT_EQ(record_.periods[2].end_ns, 400U);
    EXPECT_EQ(record_.periods[2].zones[0].nodes[0].inclusive_ns, 40U);
    EXPECT_EQ(record_.drops.outside_periods, 2U);
}

TEST_F(Small_PeriodAssembler, LingerKeepsAPeriodOpenAndALaterEventIsLate)
{
    PeriodAssembler assembler(follow(warm_one_active_two(), 100U), record_, names_, sites_, {},
                              {});

    assembler.add(batch_at(450U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U),
                                                 mark(3U, 400U)})}));
    EXPECT_EQ(record_.periods.size(), 2U);
    assembler.add(batch_at(600U, {drain_of(1U, {zone(310U, 500U), zone(150U, 520U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 3U);
    EXPECT_EQ(record_.periods[2].zones[0].nodes[0].inclusive_ns, 190U);
    ASSERT_EQ(record_.threads.size(), 2U);
    EXPECT_EQ(record_.threads[1].late, 1U);
}

TEST_F(Small_PeriodAssembler, APeriodWithoutItsEndMarkIsNotKept)
{
    WindowSchedule schedule;
    schedule.active = 5U;
    PeriodAssembler assembler(follow(schedule, 0U), record_, names_, sites_, {}, {});

    assembler.add(batch_at(300U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), zone(210U, 220U),
                                                 zone(230U, 240U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 1U);
    EXPECT_EQ(record_.periods[0].index, 0U);
    EXPECT_EQ(record_.drops.outside_periods, 2U);
}

TEST_F(Small_PeriodAssembler, AttachesBudgetsAndTags)
{
    PeriodAssembler assembler(follow(warm_one_active_two(), 0U), record_, names_, sites_, {},
                              {});
    const NameID key = names_.intern("variant");
    const NameID value = names_.intern("fused");
    const std::uint64_t tag_word = (std::uint64_t{key} << 32U) | value;

    assembler.add(batch_at(500U, {drain_of(0U, {mark(0U, 100U),
                                                 event_of(EventKind::PeriodBudget, 100U, 80U,
                                                          step_),
                                                 mark(1U, 200U),
                                                 event_of(EventKind::PeriodTag, 210U, tag_word,
                                                          step_),
                                                 mark(2U, 300U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 2U);
    EXPECT_EQ(record_.periods[0].budget_ns, 80U);
    ASSERT_EQ(record_.periods[1].tags.size(), 1U);
    EXPECT_EQ(record_.periods[1].tags[0].key, key);
    EXPECT_EQ(record_.periods[1].tags[0].value, value);
}

TEST_F(Small_PeriodAssembler, AsksSourcesForEachPeriodsSpans)
{
    StepSource source;
    SourceContext context(names_, record_.tracks, 0U);
    ASSERT_TRUE(source.begin(context).ok());
    PeriodAssembler assembler(follow(warm_one_active_two(), 0U), record_, names_, sites_,
                              {&source}, {});

    assembler.add(batch_at(500U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 2U);
    ASSERT_EQ(record_.periods[1].spans.size(), 1U);
    EXPECT_EQ(record_.periods[1].spans.span(0U).start_ns, 210U);
    EXPECT_EQ(record_.drops.spans_dropped, 2U);
}

TEST_F(Small_PeriodAssembler, StreamsPeriodsInOrderAndStopsAFailingWriter)
{
    NotingWriter good;
    NotingWriter bad;
    bad.fails = true;
    PeriodAssembler assembler(follow(warm_one_active_two(), 0U), record_, names_, sites_, {},
                              {NamedStreamWriter{"good", &good}, NamedStreamWriter{"bad", &bad}});

    assembler.add(batch_at(500U, {drain_of(0U, {mark(0U, 100U), zone(110U, 120U), mark(1U, 200U),
                                                 mark(2U, 300U), mark(3U, 400U)})}));
    assembler.finish();

    EXPECT_EQ(good.periods, "0:1 1:0 2:0 ");
    EXPECT_EQ(bad.periods, "0:1 ");
    EXPECT_EQ(assembler.problems(),
              std::vector<std::string>{"writer \"bad\" failed on period 0 and was stopped"});
}

TEST_F(Small_PeriodAssembler, FoldsLanesAndCountsOpenAndUnmatchedLanes)
{
    AssemblySettings settings;
    settings.linger_ns = 0U;
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, {});

    assembler.add(batch_at(500U, {drain_of(2U, {event_of(EventKind::LaneBegin, 10U, 0U, 7U),
                                                 zone(20U, 30U),
                                                 event_of(EventKind::LaneEnd, 40U, 0U, 7U),
                                                 event_of(EventKind::LaneEnd, 45U, 0U, 8U),
                                                 zone(50U, 60U),
                                                 event_of(EventKind::LaneBegin, 70U, 0U, 9U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods[0].zones.size(), 2U);
    EXPECT_EQ(record_.periods[0].zones[0].lane, 0U);
    EXPECT_EQ(record_.periods[0].zones[1].lane, 7U);
    EXPECT_EQ(record_.drops.open_at_stop, 1U);
    EXPECT_EQ(record_.threads[0].unmatched, 1U);
}

TEST_F(Small_PeriodAssembler, KeepsThreadsInIndexOrderWithNamesAndDrops)
{
    AssemblySettings settings;
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, {});
    ThreadDrain worker = drain_of(5U, {});
    worker.name = "worker";
    worker.dropped = 3U;
    ThreadDrain main_thread = drain_of(1U, {});
    main_thread.name = "main";

    assembler.add(batch_at(10U, {worker, main_thread}));
    worker.name.clear();
    assembler.add(batch_at(20U, {worker}));
    assembler.finish();

    ASSERT_EQ(record_.threads.size(), 2U);
    EXPECT_EQ(record_.threads[0].name, "main");
    EXPECT_EQ(record_.threads[1].name, "worker");
    EXPECT_EQ(record_.threads[1].dropped, 6U);
}
