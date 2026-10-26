/**************************************************************************/
/* session.hpp                                                            */
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
 * @file session.hpp
 * @brief Declares the profiling session: start from a configuration, mark timelines, stop.
 * @author Mustafa Garip
 * @see modules/session/session/README.md
 */

#pragma once

#include <SushiProfiler/analysis/table.hpp>
#include <SushiProfiler/config/profiler_config.hpp>
#include <SushiProfiler/consumer/consumer.hpp>
#include <SushiProfiler/consumer/pause_log.hpp>
#include <SushiProfiler/consumer/period_assembler.hpp>
#include <SushiProfiler/consumer/stream_writer_set.hpp>
#include <SushiProfiler/live/live_channel.hpp>
#include <SushiProfiler/live/live_view.hpp>
#include <SushiProfiler/live/period_publisher.hpp>
#include <SushiProfiler/periods/period_timeline.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/record_writers/record_writer.hpp>
#include <SushiProfiler/registry/registry.hpp>
#include <SushiProfiler/sources/source.hpp>
#include <SushiProfiler/stream_writers/stream_writer.hpp>
#include <SushiProfiler/version/export.hpp>
#include <SushiProfiler/window/window.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    class Session;

    /// @brief A started session, or why it did not start.
    struct SessionStart
    {
        std::unique_ptr<Session> session; /**< The session, or null. */
        std::string error;                /**< Why it did not start, or empty. */

        /// @brief Returns whether the session started.
        [[nodiscard]] bool ok() const noexcept
        {
            return session != nullptr;
        }
    };

    /// @brief What a stopped session gives back.
    struct SessionStop
    {
        RunRecord record;                  /**< The run, its newest `record.keep_periods` periods. */
        std::vector<std::string> problems; /**< What went wrong outside the record. */
    };

    /// @brief What the configuration's analyses and record writers made of a record.
    struct SessionReport
    {
        std::vector<Table> tables;         /**< The analyses' tables, in configuration order. */
        std::vector<std::string> problems; /**< Analyses refused and writers that failed. */
    };

    /// @brief What set_writer_enabled() did with a request.
    enum class WriterSwitch : std::uint8_t
    {
        Queued,        /**< The writer switches at the next period boundary. */
        UnknownWriter, /**< No writer has that name. */
        NotStreaming,  /**< The writer is a record writer, which runs once at stop. */
        NotRecording   /**< The session is not running. */
    };

    /**
     * @brief Records one run and hands back the record at stop.
     *
     * One session runs per process; every configured name is checked at start. Threads:
     * pause(), resume(), set_writer_enabled(), live(), report(), stop() and restart() share one
     * mutex, so any thread may call any of them against any other. A LiveView is safe to read
     * from any thread at any time.
     */
    class SUSHIPROFILER_API Session final
    {
        public:
            /// @brief Starts a session from @p config with the parts @p registry makes.
            [[nodiscard]] static SessionStart start(const ProfilerConfig& config,
                                                    const Registry& registry);

            Session(const Session&) = delete;
            Session& operator=(const Session&) = delete;
            Session(Session&&) = delete;
            Session& operator=(Session&&) = delete;

            /// @brief Stops the session if it still runs, dropping the record.
            ~Session();

            /// @brief Returns the timeline named @p name, making it on first use.
            [[nodiscard]] PeriodTimeline& timeline(std::string_view name);

            /// @brief Stops recording and returns the run; a second call returns an empty run.
            [[nodiscard]] SessionStop stop();

            /**
             * @brief Starts the next run on this session after stop(), with its configuration.
             *
             * Every timeline starts again at period 0, the live channel forgets the last run's
             * periods and numbers the new run, and a LiveView taken earlier reads the new run.
             * @return Empty, or why the run did not start; a refused restart leaves the session
             *         stopped.
             */
            [[nodiscard]] std::string restart();

            /// @brief Runs the configuration's analyses on @p record and its record writers.
            [[nodiscard]] SessionReport report(const RunRecord& record) const;

            /**
             * @brief Returns a view of the closed periods, and has the consumer serve it at once.
             *
             * While any copy of a view exists the consumer waits at most `live.max_poll_ms`
             * between drains and publishes each closed period.
             */
            [[nodiscard]] LiveView live();

            /**
             * @brief Stops recording zones until resume(); period marks are still written.
             * @return Whether the session paused: false when it is not running or already paused.
             *
             * Periods that overlap the pause carry the fact `paused`. Sources are not wanted while
             * paused (`Collector::sources_wanted()` is false).
             */
            [[nodiscard]] bool pause();

            /// @brief Records zones again after pause(); returns whether it took effect.
            [[nodiscard]] bool resume();

            /// @brief Switches stream writer @p name on or off at the next period boundary.
            [[nodiscard]] WriterSwitch set_writer_enabled(std::string_view name, bool enabled);

        private:
            /// @brief A stream writer the session owns, and whether it starts enabled.
            struct StreamPart
            {
                std::string name;                 /**< The configuration's name for it. */
                std::unique_ptr<StreamWriter> writer; /**< The writer. */
                bool enabled = true;              /**< Whether it receives periods at start. */
            };

            /// @brief A record writer the session owns, and whether report() runs it.
            struct RecordPart
            {
                std::string name;                 /**< The configuration's name for it. */
                std::unique_ptr<RecordWriter> writer; /**< The writer. */
                bool enabled = true;              /**< Whether report() runs it. */
            };

            /// @brief Builds an idle session of @p config.
            Session(const ProfilerConfig& config, const Registry& registry);

            /// @brief Checks the names, makes the parts, opens the collector and starts the run.
            [[nodiscard]] std::string open();

            /// @brief Makes every writer and source the configuration names.
            [[nodiscard]] std::string make_parts();

            /// @brief Opens the collector, begins the parts and starts the consumer.
            [[nodiscard]] std::string begin_recording();

            /// @brief Puts the run's facts and the clock's read cost in the record.
            void state_facts(double read_cost_ns);

            /// @brief Ends whatever open() began, in reverse order.
            std::vector<std::string> shut_down();

            /// @brief Destroys the last run's parts so open() can build the next.
            void reset_run();

            /// @brief Returns whether a run is recording; the caller holds control_mutex_.
            [[nodiscard]] bool running_locked() const noexcept;

            ProfilerConfig config_;                     /**< The configuration. */
            const Registry& registry_;                  /**< Where parts come from. */
            mutable std::mutex control_mutex_;          /**< Serialises the control calls. */
            std::shared_ptr<LiveChannel> channel_;      /**< The live channel, for the session's life. */
            std::vector<std::unique_ptr<Source>> sources_; /**< The sources. */
            std::size_t sources_begun_ = 0U;            /**< Sources whose begin succeeded. */
            std::vector<StreamPart> streams_;           /**< The stream writers. */
            std::vector<RecordPart> records_;           /**< The record writers. */
            std::unique_ptr<StreamWriterSet> writer_set_; /**< The stream writers' switches. */
            RunRecord record_;                          /**< The run being recorded. */
            std::unique_ptr<Window> window_;            /**< The window; every period without a timeline. */
            std::unique_ptr<PauseLog> pauses_;          /**< When the run was paused. */
            std::unique_ptr<PeriodPublisher> publisher_; /**< Publishes closed periods. */
            std::mutex timelines_mutex_;                /**< Guards timelines_. */
            /// @brief The timelines, by name.
            std::map<std::string, std::unique_ptr<PeriodTimeline>, std::less<>> timelines_;
            PeriodTimeline* followed_ = nullptr;        /**< The timeline the window follows. */
            std::unique_ptr<PeriodAssembler> assembler_; /**< Turns drains into periods. */
            std::unique_ptr<Consumer> consumer_;        /**< Drains on its own thread. */
            bool holds_slot_ = false;                   /**< Whether this is the running session. */
            bool opened_collector_ = false;             /**< Whether this session opened it. */
            bool paused_ = false;                       /**< Whether pause() is in effect. */
            bool stopped_ = false;                      /**< Whether stop() ran. */
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
