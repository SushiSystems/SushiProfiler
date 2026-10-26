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
#include <SushiProfiler/consumer/period_assembler.hpp>
#include <SushiProfiler/periods/period_timeline.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/record_writers/record_writer.hpp>
#include <SushiProfiler/registry/registry.hpp>
#include <SushiProfiler/sources/source.hpp>
#include <SushiProfiler/stream_writers/stream_writer.hpp>
#include <SushiProfiler/window/window.hpp>

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
        RunRecord record;                  /**< The run. */
        std::vector<std::string> problems; /**< What went wrong outside the record. */
    };

    /// @brief What the configuration's analyses and record writers made of a record.
    struct SessionReport
    {
        std::vector<Table> tables;         /**< The analyses' tables, in configuration order. */
        std::vector<std::string> problems; /**< Analyses refused and writers that failed. */
    };

    /**
     * @brief Records one run: opens the collector, follows a timeline through the window, and
     * hands back the record at stop.
     *
     * One session runs per process. Every name the configuration gives is checked at start, so a
     * typo fails before anything is recorded.
     */
    class Session final
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

            /// @brief Runs the configuration's analyses on @p record and its record writers.
            [[nodiscard]] SessionReport report(const RunRecord& record) const;

        private:
            /// @brief Builds an idle session of @p config.
            Session(const ProfilerConfig& config, const Registry& registry);

            /// @brief Makes the parts, opens the collector and starts the consumer.
            [[nodiscard]] std::string open();

            /// @brief Makes every writer and source the configuration names.
            [[nodiscard]] std::string make_parts();

            /// @brief Opens the collector, begins the parts and starts the consumer.
            [[nodiscard]] std::string begin_recording();

            /// @brief Puts the run's facts and the clock's read cost in the record.
            void state_facts(double read_cost_ns);

            /// @brief Ends whatever open() began, in reverse order.
            std::vector<std::string> shut_down();

            ProfilerConfig config_;                     /**< The configuration. */
            const Registry& registry_;                  /**< Where parts come from. */
            std::vector<std::unique_ptr<Source>> sources_; /**< The sources. */
            std::size_t sources_begun_ = 0U;            /**< Sources whose begin succeeded. */
            /// @brief The stream writers, by name.
            std::vector<std::pair<std::string, std::unique_ptr<StreamWriter>>> streams_;
            std::size_t streams_begun_ = 0U;            /**< Stream writers begun. */
            /// @brief The record writers, by name.
            std::vector<std::pair<std::string, std::unique_ptr<RecordWriter>>> records_;
            RunRecord record_;                          /**< The run being recorded. */
            std::unique_ptr<Window> window_;            /**< The window, with a timeline. */
            std::mutex timelines_mutex_;                /**< Guards timelines_. */
            /// @brief The timelines, by name.
            std::map<std::string, std::unique_ptr<PeriodTimeline>, std::less<>> timelines_;
            PeriodTimeline* followed_ = nullptr;        /**< The timeline the window follows. */
            std::unique_ptr<PeriodAssembler> assembler_; /**< Turns drains into periods. */
            std::unique_ptr<Consumer> consumer_;        /**< Drains on its own thread. */
            bool holds_slot_ = false;                   /**< Whether this is the running session. */
            bool opened_collector_ = false;             /**< Whether this session opened it. */
            bool stopped_ = false;                      /**< Whether stop() ran. */
    };
} // namespace SushiProfiler
