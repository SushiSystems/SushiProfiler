/**************************************************************************/
/* assembly_rig.hpp                                                       */
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
 * @file assembly_rig.hpp
 * @brief Declares the collaborators an assembler test needs, owned in one place.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#pragma once

#include <SushiProfiler/consumer/period_assembler.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace SushiProfilerTests
{
    /**
     * @brief Owns a window, a pause log, a live channel with its publisher and a writer set.
     *
     * The window records every period until follow() gives it a schedule; the writer set is empty
     * until set_writers(); the channel has no viewer until a test attaches one.
     */
    class AssemblyRig
    {
        public:
            /// @brief Builds a rig whose window records every period.
            AssemblyRig()
            {
                SushiProfiler::WindowSchedule all;
                all.warmup = 0U;
                all.active_all = true;
                window_ = std::make_unique<SushiProfiler::Window>(all);
                writers_ = std::make_unique<SushiProfiler::StreamWriterSet>(
                    std::vector<SushiProfiler::StreamWriterSet::Entry>{});
                publisher_ = std::make_unique<SushiProfiler::PeriodPublisher>(
                    channel_, SushiProfiler::RunCap{1'000'000U, 0U}, 0U);
            }

            /// @brief Gives the window @p schedule; call before parts().
            void follow(const SushiProfiler::WindowSchedule& schedule)
            {
                window_ = std::make_unique<SushiProfiler::Window>(schedule);
            }

            /// @brief Replaces the writers with @p entries; call before parts().
            void set_writers(std::vector<SushiProfiler::StreamWriterSet::Entry> entries)
            {
                writers_ = std::make_unique<SushiProfiler::StreamWriterSet>(std::move(entries));
            }

            /// @brief Replaces the publisher with one that keeps @p cap runs; call before parts().
            void set_run_cap(const SushiProfiler::RunCap& cap, std::uint64_t origin_steady_ns = 0U)
            {
                publisher_ = std::make_unique<SushiProfiler::PeriodPublisher>(channel_, cap,
                                                                              origin_steady_ns);
            }

            /// @brief Begins the writers enabled at start; returns the first failure or empty.
            [[nodiscard]] std::string begin_writers(const SushiProfiler::RunRecord& record)
            {
                return writers_->begin_enabled(record);
            }

            /// @brief Returns the collaborators for an assembler; the rig must outlive it.
            SushiProfiler::AssemblyParts parts()
            {
                return SushiProfiler::AssemblyParts{*window_, pauses_, *publisher_, *writers_};
            }

            /// @brief Returns the live channel.
            SushiProfiler::LiveChannel& channel() noexcept
            {
                return channel_;
            }

            /// @brief Returns the pause log.
            SushiProfiler::PauseLog& pauses() noexcept
            {
                return pauses_;
            }

            /// @brief Returns the writer set.
            SushiProfiler::StreamWriterSet& writers() noexcept
            {
                return *writers_;
            }

        private:
            std::unique_ptr<SushiProfiler::Window> window_;          /**< The window. */
            SushiProfiler::PauseLog pauses_;                         /**< The pause log. */
            SushiProfiler::LiveChannel channel_{300U};               /**< The live channel. */
            std::unique_ptr<SushiProfiler::PeriodPublisher> publisher_; /**< Its publisher. */
            std::unique_ptr<SushiProfiler::StreamWriterSet> writers_;   /**< The writer set. */
    };
} // namespace SushiProfilerTests
