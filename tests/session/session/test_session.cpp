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
 * @brief Tests a session end to end on two threads, and its refusals at start.
 * @author Mustafa Garip
 * @see tests/session/session/README.md
 */

#include <SushiProfiler/session/session.hpp>

#include <gtest/gtest.h>

#include <condition_variable>
#include <fstream>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>

#include <SushiProfiler/collector/collector.hpp>
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
    EXPECT_EQ(stopped.record.environment.back().name, "config.window");
    EXPECT_EQ(again.problems, std::vector<std::string>{"the session was already stopped"});
}
