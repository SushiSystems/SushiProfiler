/**************************************************************************/
/* session.cpp                                                            */
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
 * @file session.cpp
 * @brief Checks the configuration's names, opens the collector and assembles the run until stop.
 * @author Mustafa Garip
 * @see modules/session/session/README.md
 */

#include <SushiProfiler/session/session.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <system_error>

#include <SushiProfiler/analysis/analysis_catalog.hpp>
#include <SushiProfiler/analysis/analysis_runner.hpp>
#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/environment/environment_contributor.hpp>
#include <SushiProfiler/sites/site_registry.hpp>

namespace SushiProfiler
{
    namespace
    {
        /// @brief The longest delay a source may state, the longest linger the configuration allows.
        constexpr std::uint64_t K_MAX_SOURCE_DELAY_MS = 60'000U;

        /// @brief The one session a process may run, described for a refused second start.
        struct RunningSession
        {
            std::mutex mutex;        /**< Guards description. */
            std::string description; /**< The running session, or empty. */
        };

        /// @brief Returns the process's running-session slot.
        RunningSession& running_session()
        {
            static RunningSession slot;
            return slot;
        }

        /// @brief Returns how @p config's session is named in a refusal.
        std::string describe(const ProfilerConfig& config)
        {
            return config.timeline.empty() ? std::string("a session without a timeline")
                                           : "the session following \"" + config.timeline + "\"";
        }

        /// @brief Returns the steady clock in nanoseconds, the clock the pause log keeps.
        std::uint64_t steady_now_ns() noexcept
        {
            const auto since_epoch = std::chrono::steady_clock::now().time_since_epoch();
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch).count());
        }

        /// @brief Returns the mean cost of one read of the collector's clock, in nanoseconds.
        double measure_read_cost() noexcept
        {
            constexpr int K_READS = 1000;
            const auto begin = std::chrono::steady_clock::now();
            for (int read = 0; read < K_READS; ++read)
                static_cast<void>(Collector::now());
            const auto elapsed = std::chrono::steady_clock::now() - begin;
            const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
            return static_cast<double>(ns) / K_READS;
        }

        /// @brief Returns @p status as the words a problem uses.
        const char* status_words(WriteStatus status) noexcept
        {
            return status == WriteStatus::PathUnwritable ? "its file could not be opened"
                                                         : "its stream failed";
        }
    } // namespace

    SessionStart Session::start(const ProfilerConfig& config, const Registry& registry)
    {
        std::unique_ptr<Session> session(new Session(config, registry));
        const std::string error = session->open();
        if (!error.empty())
            return SessionStart{nullptr, error};
        return SessionStart{std::move(session), std::string()};
    }

    Session::Session(const ProfilerConfig& config, const Registry& registry)
        : config_(config), registry_(registry),
          channel_(std::make_shared<LiveChannel>(config.live.history))
    {

    }

    Session::~Session()
    {
        if (!stopped_)
            static_cast<void>(shut_down());
    }

    PeriodTimeline& Session::timeline(std::string_view name)
    {
        const std::lock_guard<std::mutex> lock(timelines_mutex_);
        const auto found = timelines_.find(name);
        if (found != timelines_.end())
            return *found->second;
        auto made = std::make_unique<PeriodTimeline>(name);
        PeriodTimeline& timeline = *made;
        timelines_.emplace(std::string(name), std::move(made));
        return timeline;
    }

    SessionStop Session::stop()
    {
        const std::lock_guard<std::mutex> lock(control_mutex_);
        if (stopped_)
            return SessionStop{RunRecord{}, {"the session was already stopped"}};
        std::vector<std::string> problems = shut_down();
        stopped_ = true;
        return SessionStop{std::move(record_), std::move(problems)};
    }

    std::string Session::restart()
    {
        const std::lock_guard<std::mutex> lock(control_mutex_);
        if (!stopped_)
            return "the session is still running; stop it before restarting";
        reset_run();
        {
            const std::lock_guard<std::mutex> timelines_lock(timelines_mutex_);
            for (const auto& entry : timelines_)
                entry.second->reset();
        }
        stopped_ = false;
        std::string error = open();
        if (!error.empty())
        {
            static_cast<void>(shut_down());
            stopped_ = true;
        }
        return error;
    }

    SessionReport Session::report(const RunRecord& record) const
    {
        const std::lock_guard<std::mutex> lock(control_mutex_);
        AnalysisCatalog catalog;
        register_builtin_analyses(catalog);
        const AnalysisRun run =
            AnalysisRunner(catalog).run(record, config_.analyses, AnalysisOptions{});
        SessionReport report;
        report.tables = run.tables;
        if (!run.error.empty())
            report.problems.push_back(run.error);
        for (const RecordPart& part : records_)
        {
            if (!part.enabled)
                continue;
            const WriteStatus status = part.writer->write(record, report.tables);
            if (status != WriteStatus::Written)
                report.problems.push_back("writer \"" + part.name + "\" failed: " +
                                          status_words(status));
        }
        return report;
    }

    LiveView Session::live()
    {
        const std::lock_guard<std::mutex> lock(control_mutex_);
        LiveView view(channel_);
        if (running_locked())
            consumer_->wake();
        return view;
    }

    bool Session::pause()
    {
        const std::lock_guard<std::mutex> lock(control_mutex_);
        if (!running_locked() || paused_)
            return false;
        pauses_->begin(steady_now_ns());
        Collector::instance().hold();
        Collector::want_sources(false);
        paused_ = true;
        return true;
    }

    bool Session::resume()
    {
        const std::lock_guard<std::mutex> lock(control_mutex_);
        if (!running_locked() || !paused_)
            return false;
        pauses_->end(steady_now_ns());
        Collector::instance().release();
        Collector::want_sources(true);
        paused_ = false;
        return true;
    }

    WriterSwitch Session::set_writer_enabled(std::string_view name, bool enabled)
    {
        const std::lock_guard<std::mutex> lock(control_mutex_);
        if (!running_locked())
            return WriterSwitch::NotRecording;
        if (writer_set_->request(name, enabled))
            return WriterSwitch::Queued;
        for (const RecordPart& part : records_)
        {
            if (part.name == name)
                return WriterSwitch::NotStreaming;
        }
        return WriterSwitch::UnknownWriter;
    }

    bool Session::running_locked() const noexcept
    {
        return consumer_ != nullptr && !stopped_;
    }

    void Session::reset_run()
    {
        consumer_.reset();
        assembler_.reset();
        publisher_.reset();
        writer_set_.reset();
        pauses_.reset();
        window_.reset();
        followed_ = nullptr;
        sources_.clear();
        sources_begun_ = 0U;
        streams_.clear();
        records_.clear();
        record_ = RunRecord{};
        opened_collector_ = false;
        paused_ = false;
    }

    std::string Session::open()
    {
        RunningSession& slot = running_session();
        {
            const std::lock_guard<std::mutex> lock(slot.mutex);
            if (!slot.description.empty())
                return "a session is already running: " + slot.description;
            slot.description = describe(config_);
            holds_slot_ = true;
        }
        static_cast<void>(channel_->begin_run());
        AnalysisCatalog catalog;
        register_builtin_analyses(catalog);
        for (const std::string& name : config_.analyses)
        {
            if (catalog.knows(name))
                continue;
            std::string error = "unknown analysis \"" + name + "\"; known:";
            const std::vector<std::string> known = catalog.names();
            for (std::size_t index = 0U; index < known.size(); ++index)
                error += (index == 0U ? " " : ", ") + known[index];
            return error;
        }
        std::string error = make_parts();
        if (!error.empty())
            return error;
        if (!config_.output.empty())
        {
            std::error_code failure;
            std::filesystem::create_directories(config_.output, failure);
            if (failure)
                return "the output folder \"" + config_.output +
                       "\" could not be made: " + failure.message();
        }
        state_facts(config_.enabled ? measure_read_cost() : 0.0);
        return config_.enabled ? begin_recording() : std::string();
    }

    std::string Session::make_parts()
    {
        for (const NamedSettings& writer : config_.writers)
        {
            if (registry_.streams(writer.name))
            {
                Made<StreamWriter> made =
                    registry_.make_stream_writer(writer.name, writer.settings, config_.output);
                if (made.part == nullptr)
                    return made.error;
                streams_.push_back(StreamPart{writer.name, std::move(made.part), writer.enabled});
                continue;
            }
            Made<RecordWriter> made =
                registry_.make_record_writer(writer.name, writer.settings, config_.output);
            if (made.part == nullptr)
                return made.error;
            records_.push_back(RecordPart{writer.name, std::move(made.part), writer.enabled});
        }
        for (const NamedSettings& source : config_.sources)
        {
            Made<Source> made = registry_.make_source(source.name, source.settings);
            if (made.part == nullptr)
                return made.error;
            sources_.push_back(std::move(made.part));
        }
        return std::string();
    }

    std::string Session::begin_recording()
    {
        Collector& collector = Collector::instance();
        if (collector.is_open())
            return "the collector is already open outside a session";
        DrainBatch leftover;
        collector.drain(leftover);
        CollectorSettings settings;
        settings.ring_events = config_.buffers.ring_events;
        settings.filter.level = config_.level;
        settings.filter.groups = config_.groups;
        if (!collector.open(settings))
            return "the collector could not be opened";
        opened_collector_ = true;
        SourceContext context(collector.names(), record_.tracks,
                              collector.first_point().steady_ns);
        std::uint64_t delay_ns = 0U;
        for (const std::unique_ptr<Source>& source : sources_)
        {
            const SourceStatus status = source->begin(context);
            if (!status.ok())
                return "source \"" + std::string(source->name()) + "\" did not start: " +
                       status.error;
            ++sources_begun_;
            const std::uint64_t source_delay_ns = source->max_delay_ns();
            if (source_delay_ns > K_MAX_SOURCE_DELAY_MS * 1'000'000U)
                return "source \"" + std::string(source->name()) + "\" states a delay of " +
                       std::to_string(source_delay_ns / 1'000'000U) + " ms; the limit is " +
                       std::to_string(K_MAX_SOURCE_DELAY_MS) + " ms";
            delay_ns = std::max(delay_ns, source_delay_ns);
        }
        Collector::want_sources(true);
        std::vector<StreamWriterSet::Entry> entries;
        for (const StreamPart& part : streams_)
            entries.push_back(StreamWriterSet::Entry{part.name, part.writer.get(), part.enabled});
        writer_set_ = std::make_unique<StreamWriterSet>(std::move(entries));
        const std::string writer_error = writer_set_->begin_enabled(record_);
        if (!writer_error.empty())
            return writer_error;
        AssemblySettings assembly;
        assembly.run_timeline = collector.names().intern("run");
        assembly.linger_ns =
            std::max(std::uint64_t{config_.buffers.linger_ms} * 1'000'000U, delay_ns);
        assembly.origin = collector.first_point();
        assembly.keep_periods = config_.record.keep_periods;
        WindowSchedule schedule = config_.window;
        if (config_.timeline.empty())
        {
            schedule = WindowSchedule{};
            schedule.warmup = 0U;
            schedule.active_all = true;
        }
        window_ = std::make_unique<Window>(schedule);
        pauses_ = std::make_unique<PauseLog>();
        publisher_ = std::make_unique<PeriodPublisher>(
            *channel_, RunCap{config_.live.max_runs, config_.live.min_run_ns},
            assembly.origin.steady_ns);
        if (!config_.timeline.empty())
        {
            collector.pause();
            followed_ = &timeline(config_.timeline);
            followed_->observe(window_.get());
            assembly.timeline = followed_->id();
        }
        std::vector<Source*> sources;
        for (const std::unique_ptr<Source>& source : sources_)
            sources.push_back(source.get());
        assembler_ = std::make_unique<PeriodAssembler>(
            assembly, record_, collector.names(), collector.sites(), std::move(sources),
            AssemblyParts{*window_, *pauses_, *publisher_, *writer_set_});
        consumer_ = std::make_unique<Consumer>(
            collector, *assembler_, *channel_,
            std::chrono::milliseconds(config_.live.max_poll_ms));
        consumer_->start();
        return std::string();
    }

    void Session::state_facts(double read_cost_ns)
    {
        const RecordFacts facts = gather_facts(registry_.contributors());
        record_.environment = facts.environment;
        record_.build = facts.build;
        const WindowSchedule& window = config_.window;
        set_fact(record_.environment, "config.timeline", config_.timeline);
        set_fact(record_.environment, "config.level", std::to_string(config_.level));
        set_fact(record_.environment, "config.window",
                 "skip " + std::to_string(window.skip) + ", wait " + std::to_string(window.wait) +
                     ", warmup " + std::to_string(window.warmup) + ", active " +
                     (window.active_all ? std::string("all") : std::to_string(window.active)) +
                     ", repeat " + std::to_string(window.repeat));
        set_fact(record_.environment, "config.record.keep_periods",
                 config_.record.keep_periods.has_value()
                     ? std::to_string(*config_.record.keep_periods)
                     : std::string("all"));
        record_.clock.read_cost_ns = read_cost_ns;
    }

    std::vector<std::string> Session::shut_down()
    {
        Collector::want_sources(false);
        if (followed_ != nullptr)
            followed_->observe(nullptr);
        if (paused_ && pauses_ != nullptr)
            pauses_->end(steady_now_ns());
        paused_ = false;
        if (opened_collector_)
            Collector::instance().close();
        opened_collector_ = false;
        if (consumer_ != nullptr)
            consumer_->stop();
        for (std::size_t index = sources_begun_; index > 0U; --index)
            sources_[index - 1U]->end();
        sources_begun_ = 0U;
        std::vector<std::string> problems;
        if (assembler_ != nullptr)
        {
            assembler_->end_writers();
            problems = assembler_->problems();
        }
        else if (writer_set_ != nullptr)
        {
            writer_set_->end_all(record_, problems);
        }
        if (holds_slot_)
        {
            RunningSession& slot = running_session();
            const std::lock_guard<std::mutex> lock(slot.mutex);
            slot.description.clear();
            holds_slot_ = false;
        }
        return problems;
    }
} // namespace SushiProfiler
