/**************************************************************************/
/* test_session.cpp                                                       */
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
 * @file test_session.cpp
 * @brief Tests a session end to end on two threads, its record's use, and its refusals.
 * @author Mustafa Garip
 * @see tests/session/session/README.md
 */

#include <SushiProfiler/session/session.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <SushiProfiler/checks/verdict.hpp>
#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/compare/compare_records.hpp>
#include <SushiProfiler/json/json_reader.hpp>
#include <SushiProfiler/record_json/record_json.hpp>
#include <SushiProfiler/zones/macros.hpp>

using SushiProfiler::Collector;
using SushiProfiler::JsonValue;
using SushiProfiler::NamedSettings;
using SushiProfiler::PeriodRecord;
using SushiProfiler::PeriodTimeline;
using SushiProfiler::ProfilerConfig;
using SushiProfiler::Registry;
using SushiProfiler::RunRecord;
using SushiProfiler::SampleSeries;
using SushiProfiler::Session;
using SushiProfiler::SessionReport;
using SushiProfiler::SessionStart;
using SushiProfiler::SessionStop;
using SushiProfiler::ZoneNode;
using SushiProfiler::ZoneTree;

namespace
{
    /// @brief Returns the value @p text holds.
    JsonValue parsed(const char* text)
    {
        return *SushiProfiler::read_json(text).value;
    }

    /// @brief Returns the text of the file at @p path.
    std::string file_text(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        std::stringstream text;
        text << file.rdbuf();
        return text.str();
    }

    /// @brief Returns the site names whose zones @p period holds.
    std::set<std::string> zone_sites(const RunRecord& record, const PeriodRecord& period)
    {
        std::set<std::string> names;
        for (const ZoneTree& tree : period.zones)
        {
            for (const ZoneNode& node : tree.nodes)
                names.insert(record.sites[node.site].name);
        }
        return names;
    }

    /// @brief Returns the site names whose samples @p period holds.
    std::set<std::string> sample_sites(const RunRecord& record, const PeriodRecord& period)
    {
        std::set<std::string> names;
        for (const SampleSeries& series : period.samples)
            names.insert(record.sites[series.site].name);
        return names;
    }

    /// @brief Runs one step of work per request on its own thread, ending each item's flow.
    class Worker final
    {
        public:
            /// @brief Starts the thread.
            Worker() : thread_(&Worker::run, this)
            {

            }

            Worker(const Worker&) = delete;
            Worker& operator=(const Worker&) = delete;
            Worker(Worker&&) = delete;
            Worker& operator=(Worker&&) = delete;

            /// @brief Stops the thread once its work is done.
            ~Worker()
            {
                {
                    const std::lock_guard<std::mutex> lock(mutex_);
                    quitting_ = true;
                }
                changed_.notify_all();
                thread_.join();
            }

            /// @brief Runs one step and waits for it.
            void step()
            {
                std::unique_lock<std::mutex> lock(mutex_);
                ++asked_;
                changed_.notify_all();
                const auto finished = [this]
                {
                    return done_ == asked_;
                };
                changed_.wait(lock, finished);
            }

        private:
            /// @brief Waits for steps and runs each.
            void run()
            {
                Collector::set_thread_name("e2e.worker");
                std::unique_lock<std::mutex> lock(mutex_);
                const auto woken = [this]
                {
                    return quitting_ || asked_ > done_;
                };
                for (;;)
                {
                    changed_.wait(lock, woken);
                    if (asked_ == done_)
                        return;
                    const std::uint64_t item = done_;
                    lock.unlock();
                    {
                        SP_ZONE("e2e.worker");
                        SP_FLOW_END("e2e.item", item);
                    }
                    lock.lock();
                    ++done_;
                    changed_.notify_all();
                }
            }

            std::mutex mutex_;                /**< Guards the counts. */
            std::condition_variable changed_; /**< Signals a count change. */
            std::uint64_t asked_ = 0U;        /**< Steps asked for. */
            std::uint64_t done_ = 0U;         /**< Steps done. */
            bool quitting_ = false;           /**< Whether the destructor runs. */
            std::thread thread_;              /**< The worker thread. */
    };

    /// @brief Returns a configuration of one warm-up and nine active `e2e.step` periods.
    ProfilerConfig end_to_end_config(const std::string& folder)
    {
        ProfilerConfig config;
        config.timeline = "e2e.step";
        config.window.warmup = 1U;
        config.window.active = 9U;
        config.analyses = {"breakdown", "counters", "flows", "overhead"};
        config.writers = {NamedSettings{"terminal", parsed(R"({"top": 5})")},
                          NamedSettings{"record", parsed(R"({"path": "run.sprec.json"})")},
                          NamedSettings{"chrome_trace", parsed(R"({"path": "run.trace.json"})")}};
        config.output = folder;
        config.buffers.linger_ms = 0U;
        return config;
    }

    /// @brief Returns a configuration that records nothing.
    ProfilerConfig disabled_config()
    {
        ProfilerConfig config;
        config.enabled = false;
        return config;
    }
} // namespace

TEST(Medium_Session, RecordsTenPeriodsFromTwoThreads)
{
    const std::string folder = ::testing::TempDir() + "sushiprofiler_session";
    std::ostringstream terminal;
    Registry registry;
    SushiProfiler::register_builtin_writers(registry, terminal);
    SessionStart started = Session::start(end_to_end_config(folder), registry);
    ASSERT_TRUE(started.ok()) << started.error;
    Session& session = *started.session;
    PeriodTimeline& step = session.timeline("e2e.step");
    Collector::set_thread_name("e2e.main");
    {
        Worker worker;
        for (std::uint64_t index = 0U; index < 11U; ++index)
        {
            static_cast<void>(step.mark());
            {
                SP_ZONE("e2e.main");
                SP_COUNTER("e2e.bytes", "B", Add, 64.0);
                SP_VALUE("e2e.level", "dB", -6.0);
                SP_INSTANT("e2e.tick");
                SP_FLOW_BEGIN("e2e.item", index);
            }
            worker.step();
        }
    }
    const SessionStop stopped = session.stop();
    const RunRecord& record = stopped.record;

    EXPECT_TRUE(stopped.problems.empty());
    ASSERT_EQ(record.periods.size(), 10U);
    EXPECT_TRUE(record.periods[0].warmup);
    for (std::size_t index = 0U; index < record.periods.size(); ++index)
    {
        const PeriodRecord& period = record.periods[index];
        EXPECT_EQ(period.index, index);
        EXPECT_EQ(zone_sites(record, period), (std::set<std::string>{"e2e.main", "e2e.worker"}));
        EXPECT_EQ(sample_sites(record, period),
                  (std::set<std::string>{"e2e.bytes", "e2e.level", "e2e.tick", "e2e.item"}));
    }
    std::set<std::string> threads;
    for (const auto& thread : record.threads)
        threads.insert(thread.name);
    EXPECT_EQ(threads.count("e2e.main"), 1U);
    EXPECT_EQ(threads.count("e2e.worker"), 1U);

    const SessionReport report = session.report(record);
    EXPECT_TRUE(report.problems.empty());
    EXPECT_EQ(report.tables.size(), 4U);
    for (const char* table : {"breakdown", "counters", "flows", "overhead"})
        EXPECT_NE(terminal.str().find(std::string("== ") + table + " =="), std::string::npos);
    ASSERT_EQ(report.tables[2].name, "flows");
    EXPECT_EQ(report.tables[2].rows.size(), 2U);
    const SushiProfiler::Comparison same = SushiProfiler::compare_records(record, record, {});
    ASSERT_TRUE(same.ok()) << same.error;
    EXPECT_FALSE(same.table.rows.empty());
    const SushiProfiler::ChecksReadResult checks = SushiProfiler::read_checks(
        R"([{"subject": "flow:e2e.item", "statistic": "max", "at_most_ns": 1},
            {"subject": "timeline:e2e.step", "statistic": "min", "at_least_ns": 1}])");
    ASSERT_TRUE(checks.ok()) << checks.error;
    const std::vector<SushiProfiler::Verdict> verdicts =
        SushiProfiler::check(record, checks.checks);
    ASSERT_EQ(verdicts.size(), 2U);
    EXPECT_FALSE(verdicts[0].passed);
    EXPECT_TRUE(verdicts[1].passed) << verdicts[1].reason;
    const SushiProfiler::RecordReadResult reread =
        SushiProfiler::read_record(file_text(folder + "/run.sprec.json"));
    ASSERT_TRUE(reread.ok()) << reread.error;
    EXPECT_TRUE(*reread.record == record);
    const SushiProfiler::JsonReadResult trace =
        SushiProfiler::read_json(file_text(folder + "/run.trace.json"));
    ASSERT_TRUE(trace.ok()) << trace.error.message;
    EXPECT_GT(trace.value->find("traceEvents")->as_array()->size(), 50U);
}

TEST(Small_Session, RefusesAnUnknownAnalysisOrWriter)
{
    Registry registry;
    std::ostringstream terminal;
    SushiProfiler::register_builtin_writers(registry, terminal);
    ProfilerConfig analysis = disabled_config();
    analysis.analyses = {"breakdown", "fastest"};
    ProfilerConfig writer = disabled_config();
    writer.writers = {NamedSettings{"csv", parsed("{}")}};

    const SessionStart refused = Session::start(analysis, registry);
    const SessionStart unknown_writer = Session::start(writer, registry);

    EXPECT_FALSE(refused.ok());
    EXPECT_EQ(refused.error.rfind("unknown analysis \"fastest\"; known: breakdown, budget", 0U),
              0U);
    EXPECT_EQ(unknown_writer.error,
              "unknown writer \"csv\"; known: chrome_trace, record, terminal");
}

TEST(Small_Session, RunsOneSessionPerProcess)
{
    const Registry registry;
    SessionStart first = Session::start(disabled_config(), registry);
    ASSERT_TRUE(first.ok()) << first.error;

    const SessionStart second = Session::start(disabled_config(), registry);
    EXPECT_EQ(second.error, "a session is already running: a session without a timeline");
    static_cast<void>(first.session->stop());
    const SessionStart third = Session::start(disabled_config(), registry);
    EXPECT_TRUE(third.ok()) << third.error;
}

TEST(Small_Session, ADisabledSessionStatesItsFactsAndRecordsNothing)
{
    const Registry registry;
    SessionStart started = Session::start(disabled_config(), registry);
    ASSERT_TRUE(started.ok());

    const SessionStop stopped = started.session->stop();
    const SessionStop again = started.session->stop();

    EXPECT_TRUE(stopped.record.periods.empty());
    EXPECT_FALSE(stopped.record.environment.empty());
    EXPECT_EQ(stopped.record.environment.back().name, "config.record.keep_periods");
    EXPECT_EQ(again.problems, std::vector<std::string>{"the session was already stopped"});
}

namespace
{
    using SushiProfiler::LiveView;
    using SushiProfiler::Made;
    using SushiProfiler::PeriodSpan;
    using SushiProfiler::Source;
    using SushiProfiler::SourceContext;
    using SushiProfiler::SourceStatus;
    using SushiProfiler::Span;
    using SushiProfiler::SpanSink;
    using SushiProfiler::WriterSwitch;

    /// @brief Waits up to five seconds for @p condition; returns whether it held.
    template <class Condition>
    bool wait_until(Condition condition)
    {
        const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (std::chrono::steady_clock::now() < end)
        {
            if (condition())
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return condition();
    }

    /// @brief Sleeps long enough for the periods around it to have length.
    void breathe()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    /**
     * @brief A source that notes when the collector wants sources, sets a fact per period, states
     * a delay and hands over queued work started in the period it is given.
     */
    class ProbeSource final : public Source
    {
        public:
            /// @brief Builds a source that states @p delay_ns.
            explicit ProbeSource(std::uint64_t delay_ns) noexcept : delay_ns_(delay_ns)
            {

            }

            /// @brief Returns `probe`.
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "probe";
            }

            /// @brief Notes the flag and adds a track and a name.
            [[nodiscard]] SourceStatus begin(SourceContext& context) override
            {
                wanted_in_begin = Collector::sources_wanted();
                track_ = context.add_track("probe.track");
                kernel_ = context.intern("probe.kernel");
                context_ = &context;
                return SourceStatus{};
            }

            /// @brief Returns the delay the source was built with.
            [[nodiscard]] std::uint64_t max_delay_ns() const noexcept override
            {
                return delay_ns_;
            }

            /// @brief Notes the flag, sets the fact and hands over the work started in @p period.
            void collect(const PeriodSpan& period, SpanSink& sink) override
            {
                if (Collector::sources_wanted())
                    wanted_collects.fetch_add(1U);
                collects.fetch_add(1U);
                static_cast<void>(sink.set_fact("probe.period", std::to_string(period.index)));
                const std::lock_guard<std::mutex> lock(mutex_);
                for (auto start = queue_.begin(); start != queue_.end();)
                {
                    if (*start >= period.end_steady_ns)
                    {
                        ++start;
                        continue;
                    }
                    if (*start < period.start_steady_ns)
                    {
                        sink.drop_late(1U);
                    }
                    else
                    {
                        Span span;
                        span.queued_ns = context_->to_record_ns(*start);
                        span.start_ns = span.queued_ns;
                        span.end_ns = span.start_ns + 1000U;
                        span.track = track_;
                        span.name = kernel_;
                        static_cast<void>(sink.append(span));
                    }
                    start = queue_.erase(start);
                }
            }

            /// @brief Notes the flag.
            void end() override
            {
                wanted_in_end = Collector::sources_wanted();
            }

            /// @brief Queues work that started at the steady time @p start_ns.
            void push(std::uint64_t start_ns)
            {
                const std::lock_guard<std::mutex> lock(mutex_);
                queue_.push_back(start_ns);
            }

            bool wanted_in_begin = true;           /**< The flag seen in begin(). */
            bool wanted_in_end = true;             /**< The flag seen in end(). */
            std::atomic<std::uint64_t> collects{0U};        /**< collect() calls. */
            std::atomic<std::uint64_t> wanted_collects{0U}; /**< collect() calls that saw the flag. */

        private:
            std::uint64_t delay_ns_;               /**< The stated delay. */
            SourceContext* context_ = nullptr;     /**< The context begin() received. */
            SushiProfiler::TrackID track_ = 0U;    /**< The probe track. */
            SushiProfiler::NameID kernel_ = 0U;    /**< The span name. */
            std::mutex mutex_;                     /**< Guards queue_. */
            std::vector<std::uint64_t> queue_;     /**< Work not yet handed over. */
    };

    /// @brief Returns the steady clock in nanoseconds.
    std::uint64_t steady_ns()
    {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count());
    }

    /// @brief Returns a configuration of every period of timeline @p timeline, no warm-up.
    ProfilerConfig live_config(const char* timeline)
    {
        ProfilerConfig config;
        config.timeline = timeline;
        config.window.warmup = 0U;
        config.window.active_all = true;
        config.buffers.linger_ms = 0U;
        return config;
    }

    /// @brief Registers a `probe` source of @p delay_ns; the last one made is stored in @p made.
    void add_probe(Registry& registry, std::uint64_t delay_ns, ProbeSource*& made)
    {
        registry.add_source("probe",
                            [delay_ns, &made](const JsonValue&)
                            {
                                auto source = std::make_unique<ProbeSource>(delay_ns);
                                made = source.get();
                                return Made<Source>{std::move(source), std::string()};
                            });
    }

    /// @brief Returns whether @p period carries fact `paused`.
    bool is_paused(const PeriodRecord& period)
    {
        for (const auto& fact : period.facts)
        {
            if (fact.name == SushiProfiler::K_PAUSED_FACT)
                return fact.value == "true";
        }
        return false;
    }
} // namespace

TEST(Medium_Session, LiveViewShowsEachClosedPeriodWhileTheSessionRuns)
{
    const Registry registry;
    SessionStart started = Session::start(live_config("live.step"), registry);
    ASSERT_TRUE(started.ok()) << started.error;
    Session& session = *started.session;
    PeriodTimeline& step = session.timeline("live.step");
    const LiveView view = session.live();
    std::uint64_t last_sequence = 0U;
    bool went_back = false;
    for (std::uint64_t index = 0U; index < 61U; ++index)
    {
        static_cast<void>(step.mark());
        {
            SP_ZONE("live.zone");
        }
        const auto latest = view.latest();
        if (latest != nullptr)
        {
            went_back = went_back || latest->sequence < last_sequence;
            last_sequence = latest->sequence;
        }
    }
    const bool arrived = wait_until(
        [&]
        {
            const auto latest = view.latest();
            return latest != nullptr && latest->period.index >= 59U;
        });
    static_cast<void>(session.stop());

    ASSERT_TRUE(arrived);
    EXPECT_FALSE(went_back);
    const auto history = view.history(10U);
    ASSERT_EQ(history.size(), 10U);
    for (std::size_t slot = 1U; slot < history.size(); ++slot)
        EXPECT_EQ(history[slot]->sequence, history[slot - 1U]->sequence + 1U);
    for (const auto& closed : history)
    {
        ASSERT_FALSE(closed->period.zones.empty());
        const auto& node = closed->period.zones[0].nodes[0];
        ASSERT_NE(closed->names->site(node.site), nullptr);
        EXPECT_EQ(closed->names->site(node.site)->name, "live.zone");
    }
}

TEST(Medium_Session, ALiveViewOutlivesTheSessionAndKeepsItsLastPeriods)
{
    const Registry registry;
    SessionStart started = Session::start(live_config("live.step"), registry);
    ASSERT_TRUE(started.ok()) << started.error;
    PeriodTimeline& step = started.session->timeline("live.step");
    const LiveView view = started.session->live();
    for (int index = 0; index < 6; ++index)
    {
        static_cast<void>(step.mark());
        SP_ZONE("live.outlives");
    }
    ASSERT_TRUE(wait_until([&] { return view.latest() != nullptr; }));
    static_cast<void>(started.session->stop());
    started.session.reset();

    const auto latest = view.latest();
    ASSERT_NE(latest, nullptr);
    EXPECT_EQ(latest->run, 1U);
    EXPECT_GT(latest->names->name(1U).size(), 0U);
    EXPECT_FALSE(view.history(10U).empty());
}

TEST(Medium_Session, AViewTakenBeforeStopKeepsWorkingAfterRestart)
{
    const Registry registry;
    SessionStart started = Session::start(live_config("live.step"), registry);
    ASSERT_TRUE(started.ok()) << started.error;
    Session& session = *started.session;
    PeriodTimeline& step = session.timeline("live.step");
    const LiveView view = session.live();
    for (int index = 0; index < 8; ++index)
        static_cast<void>(step.mark());
    ASSERT_TRUE(wait_until([&] { return view.latest() != nullptr; }));
    ASSERT_EQ(view.latest()->run, 1U);
    static_cast<void>(session.stop());

    ASSERT_EQ(session.restart(), "");
    for (int index = 0; index < 8; ++index)
        static_cast<void>(step.mark());
    const bool second = wait_until(
        [&]
        {
            const auto latest = view.latest();
            return latest != nullptr && latest->run == 2U;
        });
    static_cast<void>(session.stop());

    ASSERT_TRUE(second);
    for (const auto& closed : view.history(100U))
        EXPECT_EQ(closed->run, 2U);
    EXPECT_EQ(view.history(100U).front()->sequence, 0U);
    EXPECT_EQ(view.history(100U).front()->period.index, 0U);
}

TEST(Medium_Session, KeepsTheLastNPeriodsInTheStoppedRecord)
{
    ProfilerConfig config = live_config("keep.step");
    config.record.keep_periods = 5U;
    const Registry registry;
    SessionStart started = Session::start(config, registry);
    ASSERT_TRUE(started.ok()) << started.error;
    PeriodTimeline& step = started.session->timeline("keep.step");
    for (int index = 0; index < 31; ++index)
        static_cast<void>(step.mark());

    const SessionStop stopped = started.session->stop();

    ASSERT_EQ(stopped.record.periods.size(), 5U);
    EXPECT_EQ(stopped.record.periods.front().index, 25U);
    EXPECT_EQ(stopped.record.periods.back().index, 29U);
}

TEST(Medium_Session, KeepPeriodsZeroReturnsNoPeriodsButLiveStillPublishes)
{
    ProfilerConfig config = live_config("keep.step");
    config.record.keep_periods = 0U;
    const Registry registry;
    SessionStart started = Session::start(config, registry);
    ASSERT_TRUE(started.ok()) << started.error;
    PeriodTimeline& step = started.session->timeline("keep.step");
    const LiveView view = started.session->live();
    for (int index = 0; index < 11; ++index)
        static_cast<void>(step.mark());

    const SessionStop stopped = started.session->stop();

    EXPECT_TRUE(stopped.record.periods.empty());
    EXPECT_EQ(view.history(100U).size(), 10U);
}

TEST(Medium_Session, AWriterStartedDisabledWritesNothingUntilEnabledAtABoundary)
{
    const std::string folder = ::testing::TempDir() + "sushiprofiler_toggle";
    std::filesystem::remove_all(folder);
    std::ostringstream terminal;
    Registry registry;
    SushiProfiler::register_builtin_writers(registry, terminal);
    ProfilerConfig config = live_config("toggle.step");
    config.output = folder;
    config.writers = {NamedSettings{"chrome_trace", parsed(R"({"path": "late.trace.json"})"), false}};
    SessionStart started = Session::start(config, registry);
    ASSERT_TRUE(started.ok()) << started.error;
    Session& session = *started.session;
    PeriodTimeline& step = session.timeline("toggle.step");
    const LiveView view = session.live();
    for (int index = 0; index < 4; ++index)
    {
        static_cast<void>(step.mark());
        SP_ZONE("toggle.early");
    }
    static_cast<void>(step.mark());
    ASSERT_TRUE(wait_until(
        [&]
        {
            const auto latest = view.latest();
            return latest != nullptr && latest->period.index >= 3U;
        }));
    EXPECT_FALSE(std::filesystem::exists(folder + "/late.trace.json"));

    EXPECT_EQ(session.set_writer_enabled("chrome_trace", true), WriterSwitch::Queued);
    for (int index = 0; index < 6; ++index)
    {
        static_cast<void>(step.mark());
        SP_ZONE("toggle.late");
    }
    const SessionStop stopped = session.stop();

    EXPECT_TRUE(stopped.problems.empty());
    const std::string trace = file_text(folder + "/late.trace.json");
    EXPECT_NE(trace.find("toggle.late"), std::string::npos);
    EXPECT_EQ(trace.find("toggle.early"), std::string::npos);
}

TEST(Medium_Session, RefusesAWriterToggleForAnUnknownOrRecordWriterName)
{
    const std::string folder = ::testing::TempDir() + "sushiprofiler_refuse";
    std::ostringstream terminal;
    Registry registry;
    SushiProfiler::register_builtin_writers(registry, terminal);
    ProfilerConfig config = live_config("refuse.step");
    config.output = folder;
    config.writers = {NamedSettings{"record", parsed(R"({"path": "r.json"})")},
                      NamedSettings{"chrome_trace", parsed(R"({"path": "t.json"})")}};
    SessionStart started = Session::start(config, registry);
    ASSERT_TRUE(started.ok()) << started.error;
    Session& session = *started.session;

    EXPECT_EQ(session.set_writer_enabled("csv", true), WriterSwitch::UnknownWriter);
    EXPECT_EQ(session.set_writer_enabled("record", false), WriterSwitch::NotStreaming);
    EXPECT_EQ(session.set_writer_enabled("chrome_trace", false), WriterSwitch::Queued);
    static_cast<void>(session.stop());
    EXPECT_EQ(session.set_writer_enabled("chrome_trace", true), WriterSwitch::NotRecording);
}

TEST(Medium_SessionPause, PauseAndResumeReportWhetherTheyTookEffect)
{
    const Registry registry;
    SessionStart disabled = Session::start(disabled_config(), registry);
    ASSERT_TRUE(disabled.ok()) << disabled.error;
    EXPECT_FALSE(disabled.session->pause());
    EXPECT_FALSE(disabled.session->resume());
    static_cast<void>(disabled.session->stop());

    SessionStart started = Session::start(live_config("pause.step"), registry);
    ASSERT_TRUE(started.ok()) << started.error;
    Session& session = *started.session;
    EXPECT_FALSE(session.resume());
    EXPECT_TRUE(session.pause());
    EXPECT_FALSE(session.pause());
    EXPECT_TRUE(session.resume());
    EXPECT_FALSE(session.resume());
    static_cast<void>(session.stop());
    EXPECT_FALSE(session.pause());
}

TEST(Medium_SessionPause, PauseMarksTheCrossedPeriodsAndResumeRecordsAgain)
{
    const Registry registry;
    SessionStart started = Session::start(live_config("pause.step"), registry);
    ASSERT_TRUE(started.ok()) << started.error;
    Session& session = *started.session;
    PeriodTimeline& step = session.timeline("pause.step");

    static_cast<void>(step.mark());
    {
        SP_ZONE("pause.before");
    }
    breathe();
    static_cast<void>(step.mark());
    {
        SP_ZONE("pause.before");
    }
    breathe();
    ASSERT_TRUE(session.pause());
    breathe();
    static_cast<void>(step.mark());
    {
        SP_ZONE("pause.during");
    }
    breathe();
    static_cast<void>(step.mark());
    {
        SP_ZONE("pause.during");
    }
    breathe();
    ASSERT_TRUE(session.resume());
    static_cast<void>(step.mark());
    {
        SP_ZONE("pause.after");
    }
    breathe();
    static_cast<void>(step.mark());
    {
        SP_ZONE("pause.after");
    }
    breathe();
    static_cast<void>(step.mark());
    const SessionStop stopped = session.stop();
    const RunRecord& record = stopped.record;

    ASSERT_EQ(record.periods.size(), 6U);
    for (std::size_t index = 0U; index < record.periods.size(); ++index)
    {
        const PeriodRecord& period = record.periods[index];
        EXPECT_EQ(period.index, index);
        EXPECT_EQ(is_paused(period), index >= 1U && index <= 3U) << "period " << index;
        const std::set<std::string> zones = zone_sites(record, period);
        EXPECT_EQ(zones.count("pause.during"), 0U);
        EXPECT_EQ(zones.count("pause.after"), index >= 4U ? 1U : 0U);
        EXPECT_EQ(zones.count("pause.before"), index <= 1U ? 1U : 0U);
    }
}

TEST(Medium_SessionPause, SourcesAreNotWantedWhilePaused)
{
    ProbeSource* probe = nullptr;
    Registry registry;
    add_probe(registry, 0U, probe);
    ProfilerConfig config = live_config("pause.step");
    config.sources = {NamedSettings{"probe", parsed("{}")}};
    EXPECT_FALSE(Collector::sources_wanted());
    SessionStart started = Session::start(config, registry);
    ASSERT_TRUE(started.ok()) << started.error;
    Session& session = *started.session;
    PeriodTimeline& step = session.timeline("pause.step");
    EXPECT_TRUE(Collector::sources_wanted());
    for (int index = 0; index < 4; ++index)
        static_cast<void>(step.mark());
    ASSERT_TRUE(wait_until([&] { return probe->wanted_collects.load() > 0U; }));

    ASSERT_TRUE(session.pause());
    EXPECT_FALSE(Collector::sources_wanted());
    ASSERT_TRUE(session.resume());
    EXPECT_TRUE(Collector::sources_wanted());
    static_cast<void>(session.stop());

    EXPECT_FALSE(Collector::sources_wanted());
    ASSERT_NE(probe, nullptr);
    EXPECT_FALSE(probe->wanted_in_begin);
    EXPECT_FALSE(probe->wanted_in_end);
}

TEST(Medium_Session, ASourceDelayHoldsPeriodsOpenAndLosesNone)
{
    ProbeSource* probe = nullptr;
    Registry registry;
    add_probe(registry, 200'000'000U, probe);
    ProfilerConfig config = live_config("delay.step");
    config.sources = {NamedSettings{"probe", parsed("{}")}};
    SessionStart started = Session::start(config, registry);
    ASSERT_TRUE(started.ok()) << started.error;
    Session& session = *started.session;
    PeriodTimeline& step = session.timeline("delay.step");
    constexpr int K_PERIODS = 10;
    std::vector<std::uint64_t> starts;
    for (int index = 0; index < K_PERIODS + 4; ++index)
    {
        static_cast<void>(step.mark());
        starts.push_back(steady_ns() + 1'000'000U);
        if (index >= 3 && index - 3 < K_PERIODS)
            probe->push(starts[static_cast<std::size_t>(index - 3)]);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    const SessionStop stopped = session.stop();

    EXPECT_EQ(stopped.record.drops.spans_dropped, 0U);
    ASSERT_GE(stopped.record.periods.size(), static_cast<std::size_t>(K_PERIODS));
    for (int index = 0; index < K_PERIODS; ++index)
    {
        const PeriodRecord& period = stopped.record.periods[static_cast<std::size_t>(index)];
        EXPECT_EQ(period.index, static_cast<std::uint64_t>(index));
        EXPECT_EQ(period.spans.size(), 1U) << "period " << index;
    }
}

TEST(Small_Session, RefusesASourceDelayAboveTheLimit)
{
    ProbeSource* probe = nullptr;
    Registry registry;
    add_probe(registry, 61'000'000'000U, probe);
    ProfilerConfig config = live_config("delay.step");
    config.sources = {NamedSettings{"probe", parsed("{}")}};

    const SessionStart refused = Session::start(config, registry);

    EXPECT_FALSE(refused.ok());
    EXPECT_EQ(refused.error, "source \"probe\" states a delay of 61000 ms; the limit is 60000 ms");
    EXPECT_FALSE(Collector::sources_wanted());
}

TEST(Medium_Session, EveryPeriodWindowRecordsAllAfterWarmup)
{
    ProfilerConfig config = live_config("all.step");
    config.window.warmup = 3U;
    const Registry registry;
    SessionStart started = Session::start(config, registry);
    ASSERT_TRUE(started.ok()) << started.error;
    PeriodTimeline& step = started.session->timeline("all.step");
    for (int index = 0; index < 204; ++index)
        static_cast<void>(step.mark());

    const SessionStop stopped = started.session->stop();

    ASSERT_EQ(stopped.record.periods.size(), 203U);
    for (std::size_t index = 0U; index < stopped.record.periods.size(); ++index)
    {
        EXPECT_EQ(stopped.record.periods[index].index, index);
        EXPECT_EQ(stopped.record.periods[index].warmup, index < 3U);
    }
}

TEST(Medium_Session, ARestartAfterStopReadsASecondRecord)
{
    const Registry registry;
    SessionStart started = Session::start(live_config("again.step"), registry);
    ASSERT_TRUE(started.ok()) << started.error;
    Session& session = *started.session;
    PeriodTimeline& step = session.timeline("again.step");
    for (int index = 0; index < 4; ++index)
    {
        static_cast<void>(step.mark());
        SP_ZONE("again.first");
    }
    const SessionStop first = session.stop();

    ASSERT_EQ(session.restart(), "");
    for (int index = 0; index < 6; ++index)
    {
        static_cast<void>(step.mark());
        SP_ZONE("again.second");
    }
    const SessionStop second = session.stop();

    ASSERT_EQ(first.record.periods.size(), 3U);
    ASSERT_EQ(second.record.periods.size(), 5U);
    for (std::size_t index = 0U; index < second.record.periods.size(); ++index)
    {
        EXPECT_EQ(second.record.periods[index].index, index);
        EXPECT_EQ(zone_sites(second.record, second.record.periods[index]),
                  std::set<std::string>{"again.second"});
    }
    EXPECT_EQ(session.restart(), "");
    EXPECT_FALSE(session.restart().empty());
    static_cast<void>(session.stop());
    SessionStart third = Session::start(live_config("again.step"), registry);
    EXPECT_TRUE(third.ok()) << third.error;
}

TEST(Medium_Session, ASourceFactReachesTheStoppedRecordAndItsFile)
{
    const std::string folder = ::testing::TempDir() + "sushiprofiler_facts";
    ProbeSource* probe = nullptr;
    std::ostringstream terminal;
    Registry registry;
    SushiProfiler::register_builtin_writers(registry, terminal);
    add_probe(registry, 0U, probe);
    ProfilerConfig config = live_config("facts.step");
    config.output = folder;
    config.sources = {NamedSettings{"probe", parsed("{}")}};
    config.writers = {NamedSettings{"record", parsed(R"({"path": "facts.sprec.json"})")}};
    SessionStart started = Session::start(config, registry);
    ASSERT_TRUE(started.ok()) << started.error;
    PeriodTimeline& step = started.session->timeline("facts.step");
    for (int index = 0; index < 4; ++index)
        static_cast<void>(step.mark());
    const SessionStop stopped = started.session->stop();
    const SessionReport report = started.session->report(stopped.record);

    ASSERT_TRUE(report.problems.empty());
    ASSERT_FALSE(stopped.record.periods.empty());
    for (const PeriodRecord& period : stopped.record.periods)
    {
        ASSERT_FALSE(period.facts.empty());
        EXPECT_EQ(period.facts[0].name, "probe.period");
        EXPECT_EQ(period.facts[0].value, std::to_string(period.index));
    }
    const SushiProfiler::RecordReadResult reread =
        SushiProfiler::read_record(file_text(folder + "/facts.sprec.json"));
    ASSERT_TRUE(reread.ok()) << reread.error;
    EXPECT_TRUE(*reread.record == stopped.record);
}

TEST(Medium_Session, ControlCallsAreSerialisedAgainstStopAndRestart)
{
    const std::string folder = ::testing::TempDir() + "sushiprofiler_control";
    std::ostringstream terminal;
    Registry registry;
    SushiProfiler::register_builtin_writers(registry, terminal);
    ProfilerConfig config = live_config("control.step");
    config.output = folder;
    config.writers = {NamedSettings{"chrome_trace", parsed(R"({"path": "c.trace.json"})")}};
    SessionStart started = Session::start(config, registry);
    ASSERT_TRUE(started.ok()) << started.error;
    Session& session = *started.session;
    PeriodTimeline& step = session.timeline("control.step");
    std::atomic<bool> done{false};
    std::atomic<int> invalid{0};
    std::thread controller(
        [&]
        {
            bool enabled = false;
            while (!done.load(std::memory_order_acquire))
            {
                static_cast<void>(session.pause());
                static_cast<void>(session.resume());
                const WriterSwitch outcome = session.set_writer_enabled("chrome_trace", enabled);
                enabled = !enabled;
                if (outcome != WriterSwitch::Queued && outcome != WriterSwitch::NotRecording)
                    invalid.fetch_add(1);
                const LiveView view = session.live();
                if (!view.valid())
                    invalid.fetch_add(1);
            }
        });
    int restart_failures = 0;
    for (int round = 0; round < 15; ++round)
    {
        static_cast<void>(step.mark());
        static_cast<void>(session.stop());
        if (!session.restart().empty())
            ++restart_failures;
    }
    done.store(true, std::memory_order_release);
    controller.join();
    static_cast<void>(session.stop());

    EXPECT_EQ(invalid.load(), 0);
    EXPECT_EQ(restart_failures, 0);
}

TEST(Small_Session, RecordsKeepPeriodsInItsFacts)
{
    const Registry registry;
    ProfilerConfig all = disabled_config();
    ProfilerConfig none = disabled_config();
    none.record.keep_periods = 0U;
    ProfilerConfig seven = disabled_config();
    seven.record.keep_periods = 7U;
    std::vector<std::string> seen;
    for (const ProfilerConfig* config : {&all, &none, &seven})
    {
        SessionStart started = Session::start(*config, registry);
        ASSERT_TRUE(started.ok()) << started.error;
        const SessionStop stopped = started.session->stop();
        for (const auto& fact : stopped.record.environment)
        {
            if (fact.name == "config.record.keep_periods")
                seen.push_back(fact.value);
        }
    }

    EXPECT_EQ(seen, (std::vector<std::string>{"all", "0", "7"}));
}
