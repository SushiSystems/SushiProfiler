/**************************************************************************/
/* period_publisher.hpp                                                   */
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
 * @file period_publisher.hpp
 * @brief Declares the object the assembler calls at each close to publish the period.
 * @author Mustafa Garip
 * @see modules/session/live/README.md
 */

#pragma once

#include <SushiProfiler/live/closed_period.hpp>
#include <SushiProfiler/live/live_channel.hpp>
#include <SushiProfiler/live/name_snapshot.hpp>
#include <SushiProfiler/record/period_record.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstdint>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Publishes each closed period to a LiveChannel while a view is watching.
     *
     * It owns the name snapshot cache, the sequence and the run cap, so the assembler makes one
     * call per close and knows nothing of the live view. It runs on the consumer thread.
     */
    class SUSHIPROFILER_API PeriodPublisher final
    {
        public:
            /// @brief Builds a publisher of @p channel, which outlives it, for a run that began at
            /// @p origin_steady_ns on the steady clock.
            PeriodPublisher(LiveChannel& channel, const RunCap& cap,
                            std::uint64_t origin_steady_ns) noexcept;

            PeriodPublisher(const PeriodPublisher&) = delete;
            PeriodPublisher& operator=(const PeriodPublisher&) = delete;
            PeriodPublisher(PeriodPublisher&&) = delete;
            PeriodPublisher& operator=(PeriodPublisher&&) = delete;
            ~PeriodPublisher() = default;

            /// @brief Publishes @p period, which @p record now holds, if the channel is watched.
            void closed(const PeriodRecord& period, const RunRecord& record);

        private:
            LiveChannel& channel_;           /**< Where periods go. */
            RunCap cap_;                     /**< How many zone runs a period keeps. */
            std::uint64_t origin_steady_ns_; /**< The run's origin on the steady clock. */
            std::uint64_t sequence_ = 0U;    /**< The next period's sequence. */
            NameSnapshotCache names_;        /**< The snapshots shared between periods. */
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
