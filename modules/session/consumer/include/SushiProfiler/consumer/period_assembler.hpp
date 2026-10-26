/**************************************************************************/
/* period_assembler.hpp                                                   */
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
 * @file period_assembler.hpp
 * @brief Declares the assembler that places drained events in periods and closes them in order.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#pragma once

#include <SushiProfiler/clock/calibration.hpp>
#include <SushiProfiler/collector/drain_batch.hpp>
#include <SushiProfiler/fold/lane_tracker.hpp>
#include <SushiProfiler/fold/period_fold.hpp>
#include <SushiProfiler/fold/record_clock.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/sites/name_table.hpp>
#include <SushiProfiler/sites/site_registry.hpp>
#include <SushiProfiler/sources/source.hpp>
#include <SushiProfiler/stream_writers/stream_writer.hpp>
#include <SushiProfiler/version/export.hpp>
#include <SushiProfiler/window/window.hpp>

#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief How the assembler places events in periods.
    struct AssemblySettings
    {
        NameID timeline = 0U;          /**< The followed timeline; 0 makes the run one period. */
        NameID run_timeline = 0U;      /**< The timeline the one period carries when that is 0. */
        const Window* window = nullptr; /**< Which periods are recorded; null records all. */
        std::uint64_t linger_ns = 100'000'000U; /**< How long a period waits for late events. */
        CalibrationPoint origin;       /**< The collector's first point; record time 0. */
    };

    /// @brief A stream writer and the name the configuration gave it.
    struct NamedStreamWriter
    {
        std::string name;               /**< The writer's name. */
        StreamWriter* writer = nullptr; /**< The writer; it outlives the assembler. */
    };

    /**
     * @brief Turns drained batches into the record's periods.
     *
     * Marks open and end periods at once; other events join the period their begin falls in once
     * the drain time less the linger passes it. A period closes once that time passes its end.
     * modules/session/consumer/README.md gives the steps.
     */
    class SUSHIPROFILER_API PeriodAssembler final
    {
        public:
            /// @brief Builds an assembler that fills @p record; every reference outlives it.
            PeriodAssembler(const AssemblySettings& settings, RunRecord& record,
                            const NameTable& names, const SiteRegistry& sites,
                            std::vector<Source*> sources, std::vector<NamedStreamWriter> writers);

            PeriodAssembler(const PeriodAssembler&) = delete;
            PeriodAssembler& operator=(const PeriodAssembler&) = delete;
            PeriodAssembler(PeriodAssembler&&) = delete;
            PeriodAssembler& operator=(PeriodAssembler&&) = delete;
            ~PeriodAssembler() = default;

            /// @brief Takes one drain's events and closes the periods it lets close.
            void add(const DrainBatch& batch);

            /**
             * @brief Places every waiting event and closes every ended period.
             *
             * A period of the followed timeline with no end mark is not kept and its events count
             * as outside; the one period of a run without a timeline ends at the last drain.
             */
            void finish();

            /// @brief Returns what went wrong outside the record, such as a writer that failed.
            [[nodiscard]] const std::vector<std::string>& problems() const noexcept;

        private:
            /// @brief One recorded period still open to events.
            struct OpenPeriod
            {
                PeriodRecord period; /**< The period, without zones, samples or spans yet. */
                bool ended = false;  /**< Whether its end is known. */
                std::map<std::uint32_t, std::vector<FoldEvent>> events; /**< By thread. */
            };

            /// @brief What the assembler keeps per thread between drains.
            struct ThreadState
            {
                LaneTracker lanes;               /**< The lanes open on the thread. */
                std::deque<FoldEvent> waiting;   /**< Events not yet placed, in ring order. */
                std::uint64_t last_write_ns = 0U; /**< When its last event was written. */
            };

            /// @brief Opens and ends periods on the followed timeline's marks and budgets.
            void read_marks(const DrainBatch& batch, const RecordClock& clock);

            /// @brief Ends the open period at @p time_ns and opens period @p index if recorded.
            void mark(std::uint64_t index, std::uint32_t thread, std::uint64_t time_ns);

            /// @brief Gives @p thread's events their lanes and queues them.
            void queue(const ThreadDrain& drain, const RecordClock& clock);

            /// @brief Places @p event of @p thread in its period, or counts it late or outside.
            void place(std::uint32_t thread, const FoldEvent& event);

            /// @brief Places every waiting event that begins at or before @p horizon_ns.
            void place_waiting(std::uint64_t horizon_ns);

            /// @brief Closes every ended period whose end is at or before @p horizon_ns.
            void close_ended(std::uint64_t horizon_ns);

            /// @brief Folds @p open, collects its spans, writes it and appends it to the record.
            void close(OpenPeriod& open);

            /// @brief Returns @p thread's record, adding it in index order on first use.
            ThreadRecord& thread_record(std::uint32_t thread);

            /// @brief Copies the names and sites into the record.
            void copy_tables();

            AssemblySettings settings_;                 /**< How events are placed. */
            RunRecord& record_;                         /**< The record being filled. */
            const NameTable& names_;                    /**< The session's names. */
            const SiteRegistry& sites_;                 /**< The session's sites. */
            std::vector<Source*> sources_;              /**< The sources, asked at each close. */
            std::vector<NamedStreamWriter> writers_;    /**< The writers still writing. */
            Calibration calibration_;                   /**< The latest tick calibration. */
            std::map<std::uint32_t, ThreadState> threads_; /**< Per-thread state. */
            std::vector<OpenPeriod> open_;              /**< Periods not closed, in order. */
            std::vector<std::pair<std::uint64_t, std::uint64_t>> closed_; /**< Closed spans. */
            std::vector<std::uint64_t> reopened_ns_;    /**< When the window opened the gate. */
            std::uint64_t now_ns_ = 0U;                 /**< The last drain's time. */
            std::vector<std::string> problems_;         /**< What went wrong outside the record. */
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
