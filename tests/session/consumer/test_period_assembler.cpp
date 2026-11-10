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
 * @file test_period_assembler.cpp
 * @brief Tests placing drained events in periods: marks, the window, linger, late and outside.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/period_assembler.hpp>

#include <SushiProfiler/sources/memory_source.hpp>
#include <SushiProfiler/stream_writers/memory_stream_writer.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "assembly_rig.hpp"

using SushiProfiler::AssemblySettings;
using SushiProfiler::CounterOp;
using SushiProfiler::DrainBatch;
using SushiProfiler::EventKind;
using SushiProfiler::MemorySource;
using SushiProfiler::MemoryStreamWriter;
using SushiProfiler::MemoryWork;
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
using SushiProfiler::PeriodSpan;
using SushiProfiler::Source;
using SushiProfiler::SourceContext;
using SushiProfiler::SourceStatus;
using SushiProfiler::Span;
using SushiProfiler::SpanSink;
using SushiProfiler::StreamWriter;
using SushiProfiler::ThreadDrain;
using SushiProfiler::TraceEvent;
using SushiProfiler::TrackID;
using SushiProfiler::Window;
using SushiProfiler::WindowSchedule;
using SushiProfiler::WriteStatus;
using SushiProfiler::ZoneRun;

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
            void collect(const PeriodSpan& period, SpanSink& sink) override
            {
                Span span;
                span.start_ns = period.index * 100U + 110U;
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

    /// @brief Expects @p history to hold @p count closed periods whose sequence and period index
    /// both count up from 0 and whose names table holds `step`.
    void expect_published_in_order(
        const std::vector<std::shared_ptr<const SushiProfiler::ClosedPeriod>>& history,
        std::size_t count)
    {
        ASSERT_EQ(history.size(), count);
        for (std::size_t index = 0U; index < history.size(); ++index)
        {
            EXPECT_EQ(history[index]->sequence, index);
            EXPECT_EQ(history[index]->period.index, index);
            EXPECT_EQ(history[index]->names->name(1U), "step");
        }
    }

    /// @brief Returns each period @p writer kept as `index:runs `, in the order written.
    std::string written(const MemoryStreamWriter& writer)
    {
        std::string text;
        for (std::size_t index = 0U; index < writer.periods().size(); ++index)
        {
            text += std::to_string(writer.periods()[index].index) + ":" +
                    std::to_string(writer.instances()[index].size()) + " ";
        }
        return text;
    }

    /// @brief Returns one warm-up and two active periods, once.
    WindowSchedule warm_one_active_two()
    {
        WindowSchedule schedule;
        schedule.warmup = 1U;
        schedule.active = 2U;
        return schedule;
    }

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
                rig_.follow(schedule);
                AssemblySettings settings;
                settings.timeline = step_;
                settings.linger_ns = linger_ns;
                return settings;
            }

            /// @brief Returns a mark of `step` opening period @p index at @p time.
            TraceEvent mark(std::uint64_t index, std::uint64_t time) const
            {
                return event_of(EventKind::PeriodMark, time, index, step_);
            }

            /// @brief Runs a lingering session: four marks, then a late and a still-open event.
            void run_lingering_session()
            {
                const AssemblySettings settings = follow(warm_one_active_two(), 100U);
                PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());
                assembler.add(batch_at(450U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U),
                                                             mark(2U, 300U), mark(3U, 400U)})}));
                afterwards_ = record_.periods.size();
                assembler.add(batch_at(600U, {drain_of(1U, {zone(310U, 500U), zone(150U, 520U)})}));
                assembler.finish();
            }

            /// @brief Runs a session whose first period has a budget and whose second has a tag.
            void run_budget_and_tag()
            {
                const AssemblySettings settings = follow(warm_one_active_two(), 0U);
                PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());
                const NameID key = names_.intern("variant");
                const NameID value = names_.intern("fused");
                const std::uint64_t tag_word = (std::uint64_t{key} << 32U) | value;
                assembler.add(batch_at(
                    500U, {drain_of(0U, {mark(0U, 100U),
                                         event_of(EventKind::PeriodBudget, 100U, 80U, step_),
                                         mark(1U, 200U),
                                         event_of(EventKind::PeriodTag, 210U, tag_word, step_),
                                         mark(2U, 300U)})}));
                assembler.finish();
            }

            /// @brief Runs a thread that opens a lane, closes it, closes an unknown one and opens
            /// one it never closes.
            void run_lanes()
            {
                AssemblySettings settings;
                settings.linger_ns = 0U;
                PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());
                assembler.add(batch_at(
                    500U, {drain_of(2U, {event_of(EventKind::LaneBegin, 10U, 0U, 7U), zone(20U, 30U),
                                         event_of(EventKind::LaneEnd, 40U, 0U, 7U),
                                         event_of(EventKind::LaneEnd, 45U, 0U, 8U), zone(50U, 60U),
                                         event_of(EventKind::LaneBegin, 70U, 0U, 9U)})}));
                assembler.finish();
            }

            /// @brief Drains thread 5 named "worker" with 3 drops, thread 1 named "main", then
            /// thread 5 again with no name.
            void run_threads()
            {
                AssemblySettings settings;
                PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());
                ThreadDrain worker = drain_of(5U, {});
                worker.name = "worker";
                worker.dropped = 3U;
                ThreadDrain main_thread = drain_of(1U, {});
                main_thread.name = "main";
                assembler.add(batch_at(10U, {worker, main_thread}));
                worker.name.clear();
                assembler.add(batch_at(20U, {worker}));
                assembler.finish();
            }

            /// @brief Returns @p count marks of `step`; mark n sits at 10 times n.
            std::vector<TraceEvent> marks_every_ten(std::uint64_t count) const
            {
                std::vector<TraceEvent> marks;
                for (std::uint64_t index = 0U; index < count; ++index)
                    marks.push_back(mark(index, 10U * index));
                return marks;
            }

            /// @brief Returns the late events summed over the threads of the record.
            std::uint64_t late_total() const
            {
                std::uint64_t late = 0U;
                for (const auto& thread : record_.threads)
                    late += thread.late;
                return late;
            }

            NameTable names_;                   /**< The session's names. */
            SiteRegistry sites_;                /**< The session's sites. */
            SiteHandle conv_{&K_CONV};          /**< Site 1's handle. */
            NameID step_ = 0U;                  /**< The followed timeline. */
            SushiProfilerTests::AssemblyRig rig_; /**< The collaborators of the assembler. */
            RunRecord record_;                  /**< The record filled. */
            std::size_t afterwards_ = 0U;       /**< Periods the record held mid-session. */
    };
} // namespace

TEST_F(Small_PeriodAssembler, RecordsARunWithoutATimelineAsOnePeriod)
{
    AssemblySettings settings;
    settings.run_timeline = step_;
    settings.linger_ns = 0U;
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

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
}

TEST_F(Small_PeriodAssembler, CopiesTheNamesAndSitesIntoTheRecord)
{
    AssemblySettings settings;
    settings.run_timeline = step_;
    settings.linger_ns = 0U;
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(500U, {drain_of(0U, {zone(10U, 20U)})}));
    assembler.finish();

    EXPECT_EQ(record_.names, (std::vector<std::string>{"", "step"}));
    ASSERT_EQ(record_.sites.size(), 2U);
    EXPECT_EQ(record_.sites[1].group, "Model");
}

TEST_F(Small_PeriodAssembler, KeepsTheWindowsPeriodsWithTheirWarmupFlags)
{
    const AssemblySettings settings = follow(warm_one_active_two(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(
        1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U), mark(3U, 400U)}),
                 drain_of(1U, {zone(150U, 160U), zone(250U, 260U), zone(350U, 390U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 3U);
    EXPECT_TRUE(record_.periods[0].warmup);
    EXPECT_FALSE(record_.periods[1].warmup);
    EXPECT_EQ(record_.periods[2].begin_ns, 300U);
    EXPECT_EQ(record_.periods[2].end_ns, 400U);
    EXPECT_EQ(record_.periods[2].zones[0].nodes[0].inclusive_ns, 40U);
}

TEST_F(Small_PeriodAssembler, CountsEventsOutsideTheWindowAsOutside)
{
    const AssemblySettings settings = follow(warm_one_active_two(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(
        1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U), mark(3U, 400U)}),
                 drain_of(1U, {zone(450U, 460U), zone(50U, 60U)})}));
    assembler.finish();

    EXPECT_EQ(record_.drops.outside_periods, 2U);
}

TEST_F(Small_PeriodAssembler, LingerKeepsAPeriodOpenAfterItsEndMark)
{
    run_lingering_session();

    EXPECT_EQ(afterwards_, 2U);
    ASSERT_EQ(record_.periods.size(), 3U);
    EXPECT_EQ(record_.periods[2].zones[0].nodes[0].inclusive_ns, 190U);
}

TEST_F(Small_PeriodAssembler, AnEventAfterTheLingerIsCountedLate)
{
    run_lingering_session();

    ASSERT_EQ(record_.threads.size(), 2U);
    EXPECT_EQ(record_.threads[1].late, 1U);
}

TEST_F(Small_PeriodAssembler, APeriodWithoutItsEndMarkIsNotKept)
{
    WindowSchedule schedule;
    schedule.active = 5U;
    const AssemblySettings settings = follow(schedule, 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(300U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), zone(210U, 220U),
                                                 zone(230U, 240U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 1U);
    EXPECT_EQ(record_.periods[0].index, 0U);
    EXPECT_EQ(record_.drops.outside_periods, 2U);
}

TEST_F(Small_PeriodAssembler, AttachesABudgetToItsPeriod)
{
    run_budget_and_tag();

    ASSERT_EQ(record_.periods.size(), 2U);
    EXPECT_EQ(record_.periods[0].budget_ns, 80U);
}

TEST_F(Small_PeriodAssembler, AttachesATagToItsPeriod)
{
    run_budget_and_tag();

    ASSERT_EQ(record_.periods.size(), 2U);
    ASSERT_EQ(record_.periods[1].tags.size(), 1U);
    EXPECT_EQ(record_.periods[1].tags[0].key, names_.intern("variant"));
    EXPECT_EQ(record_.periods[1].tags[0].value, names_.intern("fused"));
}

TEST_F(Small_PeriodAssembler, AsksSourcesForEachPeriodsSpans)
{
    StepSource source;
    SourceContext context(names_, record_.tracks, 0U);
    ASSERT_TRUE(source.begin(context).ok());
    const AssemblySettings settings = follow(warm_one_active_two(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {&source}, rig_.parts());

    assembler.add(batch_at(500U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 2U);
    ASSERT_EQ(record_.periods[1].spans.size(), 1U);
    EXPECT_EQ(record_.periods[1].spans.span(0U).start_ns, 210U);
    EXPECT_EQ(record_.drops.spans_dropped, 2U);
}

TEST_F(Small_PeriodAssembler, GivesSourcesEachPeriodsSteadySpan)
{
    MemorySource source("placing");
    SourceContext context(names_, record_.tracks, 1'000U);
    ASSERT_TRUE(source.begin(context).ok());
    AssemblySettings settings = follow(warm_one_active_two(), 0U);
    settings.origin = {1'000U, 1'000U};
    PeriodAssembler assembler(settings, record_, names_, sites_, {&source}, rig_.parts());

    assembler.add(batch_at(1'500U, {drain_of(0U, {mark(0U, 1'100U), mark(1U, 1'200U),
                                                   mark(2U, 1'300U)})}));
    assembler.finish();

    const std::vector<PeriodSpan> spans = source.collected();
    ASSERT_EQ(spans.size(), 2U);
    EXPECT_EQ(spans[0].index, 0U);
    EXPECT_EQ(spans[0].start_steady_ns, 1'100U);
    EXPECT_EQ(spans[0].end_steady_ns, 1'200U);
    EXPECT_EQ(spans[1].index, 1U);
    EXPECT_EQ(spans[1].start_steady_ns, 1'200U);
    EXPECT_EQ(spans[1].end_steady_ns, 1'300U);
}

TEST_F(Small_PeriodAssembler, ASourceInternsANameAndAddsATrackWhileCollecting)
{
    MemorySource source("placing");
    source.queue(MemoryWork{"device_0", "work_0", 150U, 150U, 155U});
    SourceContext context(names_, record_.tracks, 0U);
    ASSERT_TRUE(source.begin(context).ok());
    const AssemblySettings settings = follow(warm_one_active_two(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {&source}, rig_.parts());

    assembler.add(batch_at(500U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods[0].spans.size(), 1U);
    EXPECT_EQ(record_.names[record_.periods[0].spans.span(0U).name], "work_0");
    EXPECT_EQ(record_.tracks[record_.periods[0].spans.span(0U).track], "device_0");
    EXPECT_EQ(record_.drops.spans_dropped, 0U);
}

TEST_F(Small_PeriodAssembler, PlacesWorkInThePeriodHoldingItsStartAndCountsOlderWorkAsDropped)
{
    MemorySource source("placing");
    source.queue(MemoryWork{"device", "work", 50U, 50U, 55U});
    source.queue(MemoryWork{"device", "work", 150U, 150U, 155U});
    source.queue(MemoryWork{"device", "work", 250U, 250U, 255U});
    SourceContext context(names_, record_.tracks, 0U);
    ASSERT_TRUE(source.begin(context).ok());
    const AssemblySettings settings = follow(warm_one_active_two(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {&source}, rig_.parts());

    assembler.add(batch_at(500U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 2U);
    ASSERT_EQ(record_.periods[0].spans.size(), 1U);
    EXPECT_EQ(record_.periods[0].spans.span(0U).start_ns, 150U);
    ASSERT_EQ(record_.periods[1].spans.size(), 1U);
    EXPECT_EQ(record_.periods[1].spans.span(0U).start_ns, 250U);
    EXPECT_EQ(record_.drops.spans_dropped, 1U);
}

TEST_F(Small_PeriodAssembler, KeepsTheZoneRunsOfAnActivePeriodButNotOfAWarmupOne)
{
    const AssemblySettings settings = follow(warm_one_active_two(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(500U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U)}),
                                  drain_of(1U, {zone(110U, 120U), zone(250U, 280U),
                                                zone(255U, 270U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 2U);
    EXPECT_TRUE(record_.periods[0].runs.empty());
    ASSERT_EQ(record_.periods[1].runs.size(), 2U);
    EXPECT_EQ(record_.periods[1].runs[0].thread, 1U);
    EXPECT_EQ(record_.periods[1].runs[0].site, 1U);
    EXPECT_EQ(record_.periods[1].runs[0].begin_ns, 250U);
    EXPECT_EQ(record_.periods[1].runs[0].end_ns, 280U);
    EXPECT_EQ(record_.periods[1].runs[0].depth, 0U);
    EXPECT_EQ(record_.periods[1].runs[1].depth, 1U);
}

TEST_F(Small_PeriodAssembler, StreamsPeriodsInOrderAndStopsAFailingWriter)
{
    MemoryStreamWriter good;
    MemoryStreamWriter bad;
    rig_.set_writers({{"good", &good, true}, {"bad", &bad, true}});
    ASSERT_EQ(rig_.begin_writers(record_), "");
    bad.fail_with(WriteStatus::StreamFailed);
    const AssemblySettings settings = follow(warm_one_active_two(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(500U, {drain_of(0U, {mark(0U, 100U), zone(110U, 120U), mark(1U, 200U),
                                                 mark(2U, 300U), mark(3U, 400U)})}));
    assembler.finish();

    EXPECT_EQ(written(good), "0:1 1:0 2:0 ");
    EXPECT_EQ(bad.writes(), 1U);
    EXPECT_EQ(assembler.problems(),
              std::vector<std::string>{"writer \"bad\" failed on period 0 and was stopped"});
}

TEST_F(Small_PeriodAssembler, FoldsALaneIntoItsOwnZoneTree)
{
    run_lanes();

    ASSERT_EQ(record_.periods[0].zones.size(), 2U);
    EXPECT_EQ(record_.periods[0].zones[0].lane, 0U);
    EXPECT_EQ(record_.periods[0].zones[1].lane, 7U);
}

TEST_F(Small_PeriodAssembler, CountsALaneStillOpenAtStop)
{
    run_lanes();

    EXPECT_EQ(record_.drops.open_at_stop, 1U);
}

TEST_F(Small_PeriodAssembler, CountsALaneEndThatMatchesNoBegin)
{
    run_lanes();

    EXPECT_EQ(record_.threads[0].unmatched, 1U);
}

TEST_F(Small_PeriodAssembler, KeepsThreadsInIndexOrder)
{
    run_threads();

    ASSERT_EQ(record_.threads.size(), 2U);
    EXPECT_EQ(record_.threads[0].name, "main");
    EXPECT_EQ(record_.threads[1].name, "worker");
}

TEST_F(Small_PeriodAssembler, KeepsTheFirstThreadNameOverALaterEmptyOne)
{
    run_threads();

    ASSERT_EQ(record_.threads.size(), 2U);
    EXPECT_EQ(record_.threads[1].name, "worker");
}

TEST_F(Small_PeriodAssembler, SumsTheDropsOfAThreadAcrossBatches)
{
    run_threads();

    ASSERT_EQ(record_.threads.size(), 2U);
    EXPECT_EQ(record_.threads[1].dropped, 6U);
}

namespace
{
    /// @brief Returns a schedule that records every period from the first, none as warm-up.
    WindowSchedule every_period()
    {
        WindowSchedule schedule;
        schedule.warmup = 0U;
        schedule.active_all = true;
        return schedule;
    }

    /// @brief A stream writer that notes what the live channel and the record held at each write.
    class ProbingWriter final : public StreamWriter
    {
        public:
            /// @brief Builds a writer that reads @p channel at each write.
            explicit ProbingWriter(const SushiProfiler::LiveChannel& channel) : channel_(channel)
            {

            }

            /// @brief Starts.
            [[nodiscard]] WriteStatus begin(const RunRecord& record) override
            {
                static_cast<void>(record);
                return WriteStatus::Written;
            }

            /// @brief Notes the newest published sequence (-1 for none) and the record's periods.
            [[nodiscard]] WriteStatus write_period(
                const RunRecord& record, const PeriodRecord& period,
                const std::vector<ZoneRun>& instances) override
            {
                static_cast<void>(period);
                static_cast<void>(instances);
                const auto latest = channel_.latest();
                published.push_back(latest == nullptr ? -1
                                                      : static_cast<std::int64_t>(latest->sequence));
                recorded.push_back(record.periods.size());
                return WriteStatus::Written;
            }

            /// @brief Ends.
            [[nodiscard]] WriteStatus end(const RunRecord& record) override
            {
                static_cast<void>(record);
                return WriteStatus::Written;
            }

            std::vector<std::int64_t> published; /**< The newest sequence at each write. */
            std::vector<std::size_t> recorded;   /**< The record's period count at each write. */

        private:
            const SushiProfiler::LiveChannel& channel_; /**< The channel read. */
    };
} // namespace

TEST_F(Small_PeriodAssembler, KeepsTheLastNPeriodsAtStop)
{
    AssemblySettings settings = follow(every_period(), 0U);
    settings.keep_periods = 2U;
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U),
                                                   mark(3U, 400U), mark(4U, 500U),
                                                   mark(5U, 600U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 2U);
    EXPECT_EQ(record_.periods[0].index, 3U);
    EXPECT_EQ(record_.periods[1].index, 4U);
}

TEST_F(Small_PeriodAssembler, KeepsNoPeriodsWhenZeroButStillStreamsThem)
{
    MemoryStreamWriter writer;
    rig_.set_writers({{"noting", &writer, true}});
    ASSERT_EQ(rig_.begin_writers(record_), "");
    AssemblySettings settings = follow(every_period(), 0U);
    settings.keep_periods = 0U;
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U),
                                                   mark(3U, 400U)})}));
    assembler.finish();

    EXPECT_TRUE(record_.periods.empty());
    EXPECT_EQ(written(writer), "0:0 1:0 2:0 ");
}

TEST_F(Small_PeriodAssembler, KeepsEveryPeriodWhenAbsent)
{
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U),
                                                   mark(3U, 400U), mark(4U, 500U)})}));
    assembler.finish();

    EXPECT_EQ(record_.periods.size(), 4U);
}

TEST_F(Small_PeriodAssembler, ALateEventOfATrimmedPeriodIsStillCountedLate)
{
    AssemblySettings settings = follow(every_period(), 0U);
    settings.keep_periods = 1U;
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());
    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U),
                                                   mark(3U, 400U)})}));

    assembler.add(batch_at(2'000U, {drain_of(1U, {zone(120U, 130U)})}));
    assembler.finish();

    EXPECT_EQ(record_.periods.size(), 1U);
    EXPECT_EQ(late_total(), 1U);
    EXPECT_EQ(record_.drops.outside_periods, 0U);
}

TEST_F(Small_PeriodAssembler, ALateEventOlderThanTheTrackedSpansCountsAsOutside)
{
    WindowSchedule gappy;
    gappy.wait = 1U;
    gappy.warmup = 0U;
    gappy.active = 1U;
    gappy.repeat = 0U;
    const AssemblySettings settings = follow(gappy, 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());
    assembler.add(batch_at(1'000'000U, {drain_of(0U, marks_every_ten(8'402U))}));
    ASSERT_EQ(record_.periods.size(), 4'200U);

    assembler.add(batch_at(2'000'000U, {drain_of(1U, {zone(15U, 16U), zone(83'995U, 83'996U)})}));
    assembler.finish();

    EXPECT_EQ(record_.drops.outside_periods, 1U);
    EXPECT_EQ(late_total(), 1U);
}

TEST_F(Small_PeriodAssembler, TheRecordsNamesFollowTheTableBetweenBatches)
{
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());
    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U)})}));
    ASSERT_EQ(record_.names, (std::vector<std::string>{"", "step"}));

    static_cast<void>(names_.intern("later"));
    assembler.add(batch_at(2'000U, {drain_of(0U, {mark(2U, 300U)})}));

    EXPECT_EQ(record_.names, (std::vector<std::string>{"", "step", "later"}));
}

TEST_F(Small_PeriodAssembler, TheRecordsSitesFollowTheRegistryBetweenBatches)
{
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U)})}));

    ASSERT_EQ(record_.sites.size(), 2U);
    EXPECT_EQ(record_.sites[1].name, "conv");
}

TEST_F(Small_PeriodAssembler, ASourceFactReachesThePeriodRecord)
{
    MemorySource source("facts");
    source.state_fact("layer", "graph");
    SourceContext context(names_, record_.tracks, 0U);
    ASSERT_TRUE(source.begin(context).ok());
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {&source}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 1U);
    ASSERT_EQ(record_.periods[0].facts.size(), 1U);
    EXPECT_EQ(record_.periods[0].facts[0].name, "layer");
    EXPECT_EQ(record_.periods[0].facts[0].value, "graph");
}

TEST_F(Small_PeriodAssembler, MarksAPeriodThatOverlapsAPauseAndNoOther)
{
    rig_.pauses().begin(250U);
    rig_.pauses().end(260U);
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U),
                                                   mark(3U, 400U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 3U);
    EXPECT_TRUE(record_.periods[0].facts.empty());
    ASSERT_EQ(record_.periods[1].facts.size(), 1U);
    EXPECT_EQ(record_.periods[1].facts[0].name, SushiProfiler::K_PAUSED_FACT);
    EXPECT_EQ(record_.periods[1].facts[0].value, "true");
    EXPECT_TRUE(record_.periods[2].facts.empty());
}

TEST_F(Small_PeriodAssembler, APausedPeriodKeepsItsMarksAndHasNoZones)
{
    rig_.pauses().begin(100U);
    rig_.pauses().end(200U);
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U),
                                                   zone(210U, 220U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 2U);
    EXPECT_EQ(record_.periods[0].index, 0U);
    EXPECT_TRUE(record_.periods[0].zones.empty());
    EXPECT_EQ(record_.periods[0].facts.size(), 1U);
    EXPECT_EQ(record_.periods[1].zones.size(), 1U);
    EXPECT_TRUE(record_.periods[1].facts.empty());
}

TEST_F(Small_PeriodAssembler, ASourceStillCollectsAPausedPeriod)
{
    rig_.pauses().begin(100U);
    rig_.pauses().end(200U);
    MemorySource source("placing");
    SourceContext context(names_, record_.tracks, 0U);
    ASSERT_TRUE(source.begin(context).ok());
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {&source}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U)})}));
    assembler.finish();

    const std::vector<PeriodSpan> spans = source.collected();
    ASSERT_EQ(spans.size(), 2U);
    EXPECT_EQ(spans[0].index, 0U);
}

TEST_F(Small_PeriodAssembler, ALaneOpenAcrossAPauseIsResetAfterResume)
{
    rig_.pauses().begin(150U);
    rig_.pauses().end(160U);
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(
        1'000U, {drain_of(0U, {mark(0U, 100U), event_of(EventKind::LaneBegin, 110U, 0U, 7U),
                               zone(120U, 130U), zone(170U, 180U),
                               event_of(EventKind::LaneEnd, 190U, 0U, 7U), mark(1U, 300U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 1U);
    const PeriodRecord& period = record_.periods[0];
    ASSERT_EQ(period.zones.size(), 2U);
    EXPECT_EQ(period.zones[0].lane, 0U);
    EXPECT_EQ(period.zones[1].lane, 7U);
    EXPECT_EQ(record_.threads[0].unmatched, 1U);
    EXPECT_EQ(record_.drops.open_at_stop, 0U);
}

namespace
{
    /// @brief One forget case: the drains of other threads and the pause intervals that remain.
    struct ForgetRow
    {
        std::vector<ThreadDrain> others;
        std::size_t remaining;
    };

    /// @brief Returns a thread 5 that wrote at 5 ns and retired.
    ThreadDrain retired_thread()
    {
        ThreadDrain gone = drain_of(5U, {zone(5U, 10U)});
        gone.retired = true;
        return gone;
    }

    /// @brief Holds one forget row on top of the assembler fixture.
    class Small_PeriodAssemblerForget : public Small_PeriodAssembler,
                                        public ::testing::WithParamInterface<ForgetRow>
    {
    };
} // namespace

TEST_P(Small_PeriodAssemblerForget, AnOldPauseIntervalIsKeptOnlyWhileALiveThreadWroteEarlier)
{
    rig_.pauses().begin(20U);
    rig_.pauses().end(30U);
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());
    std::vector<ThreadDrain> threads = {drain_of(0U, {mark(0U, 100U), zone(110U, 120U),
                                                       mark(1U, 200U), mark(2U, 300U)})};
    threads.insert(threads.end(), GetParam().others.begin(), GetParam().others.end());

    assembler.add(batch_at(1'000U, threads));

    EXPECT_EQ(rig_.pauses().ends_after(0U).size(), GetParam().remaining);
}

INSTANTIATE_TEST_SUITE_P(Small_Forget, Small_PeriodAssemblerForget,
                         ::testing::Values(ForgetRow{{}, 0U},
                                           ForgetRow{{drain_of(5U, {zone(5U, 10U)})}, 1U},
                                           ForgetRow{{retired_thread()}, 0U}));

TEST_F(Small_PeriodAssembler, AWriterEnabledBeforeACloseReceivesThatPeriod)
{
    MemoryStreamWriter writer;
    rig_.set_writers({{"noting", &writer, false}});
    ASSERT_EQ(rig_.begin_writers(record_), "");
    EXPECT_EQ(writer.begins(), 0U);
    ASSERT_TRUE(rig_.writers().request("noting", true));
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U)})}));
    assembler.finish();

    EXPECT_EQ(writer.begins(), 1U);
    EXPECT_EQ(written(writer), "0:0 ");
}

TEST_F(Small_PeriodAssembler, AWriterDisabledBeforeACloseDoesNotReceiveIt)
{
    MemoryStreamWriter writer;
    rig_.set_writers({{"noting", &writer, true}});
    ASSERT_EQ(rig_.begin_writers(record_), "");
    ASSERT_TRUE(rig_.writers().request("noting", false));
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U)})}));
    assembler.finish();

    EXPECT_TRUE(writer.periods().empty());
    EXPECT_EQ(writer.begins(), 1U);
}

TEST_F(Small_PeriodAssembler, ATogglePostedWhileAPeriodIsOpenTakesEffectAtTheNextClose)
{
    MemoryStreamWriter writer;
    rig_.set_writers({{"noting", &writer, true}});
    ASSERT_EQ(rig_.begin_writers(record_), "");
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());
    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U)})}));
    ASSERT_EQ(written(writer), "0:0 ");

    ASSERT_TRUE(rig_.writers().request("noting", false));
    assembler.add(batch_at(2'000U, {drain_of(0U, {mark(2U, 300U)})}));
    assembler.finish();

    EXPECT_EQ(written(writer), "0:0 ");
}

TEST_F(Small_PeriodAssembler, PublishesEachClosedPeriodWhenAViewIsWatching)
{
    rig_.channel().attach();
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U),
                                                   mark(3U, 400U)})}));
    assembler.finish();

    expect_published_in_order(rig_.channel().history(10U), 3U);
}

TEST_F(Small_PeriodAssembler, PublishesNothingWithoutAView)
{
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U)})}));
    assembler.finish();

    EXPECT_EQ(rig_.channel().published(), 0U);
    EXPECT_EQ(record_.periods.size(), 2U);
}

TEST_F(Small_PeriodAssembler, TheClosedPeriodCapsItsRunsButTheRecordKeepsThemAll)
{
    rig_.set_run_cap(SushiProfiler::RunCap{1U, 0U});
    rig_.channel().attach();
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), zone(110U, 120U),
                                                   zone(130U, 160U), zone(170U, 175U),
                                                   mark(1U, 200U)})}));
    assembler.finish();

    ASSERT_EQ(record_.periods.size(), 1U);
    EXPECT_EQ(record_.periods[0].runs.size(), 3U);
    const auto latest = rig_.channel().latest();
    ASSERT_NE(latest, nullptr);
    EXPECT_EQ(latest->period.runs.size(), 1U);
    EXPECT_EQ(latest->runs_omitted, 2U);
    EXPECT_EQ(latest->period.runs[0].end_ns - latest->period.runs[0].begin_ns, 30U);
}

TEST_F(Small_PeriodAssembler, AClosedPeriodOutlivesTheAssembler)
{
    rig_.channel().attach();
    std::shared_ptr<const SushiProfiler::ClosedPeriod> kept;
    {
        const AssemblySettings settings = follow(every_period(), 0U);
        PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());
        assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), zone(110U, 120U),
                                                       mark(1U, 200U)})}));
        assembler.finish();
        kept = rig_.channel().latest();
    }
    record_ = RunRecord{};

    ASSERT_NE(kept, nullptr);
    EXPECT_EQ(kept->names->name(1U), "step");
    EXPECT_EQ(kept->period.zones.size(), 1U);
}

TEST_F(Small_PeriodAssembler, PublishesAfterTheWritersAndTheRecord)
{
    rig_.channel().attach();
    ProbingWriter writer(rig_.channel());
    rig_.set_writers({{"probe", &writer, true}});
    ASSERT_EQ(rig_.begin_writers(record_), "");
    const AssemblySettings settings = follow(every_period(), 0U);
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U),
                                                   mark(3U, 400U)})}));
    assembler.finish();

    EXPECT_EQ(writer.published, (std::vector<std::int64_t>{-1, 0, 1}));
    EXPECT_EQ(writer.recorded, (std::vector<std::size_t>{0U, 1U, 2U}));
    EXPECT_EQ(rig_.channel().latest()->sequence, 2U);
}

TEST_F(Small_PeriodAssembler, KeepPeriodsZeroStillPublishesEachPeriod)
{
    rig_.channel().attach();
    AssemblySettings settings = follow(every_period(), 0U);
    settings.keep_periods = 0U;
    PeriodAssembler assembler(settings, record_, names_, sites_, {}, rig_.parts());

    assembler.add(batch_at(1'000U, {drain_of(0U, {mark(0U, 100U), mark(1U, 200U), mark(2U, 300U),
                                                   mark(3U, 400U)})}));
    assembler.finish();

    EXPECT_TRUE(record_.periods.empty());
    EXPECT_EQ(rig_.channel().history(10U).size(), 3U);
}
