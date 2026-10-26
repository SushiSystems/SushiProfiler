/**************************************************************************/
/* live_view.hpp                                                          */
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
 * @file live_view.hpp
 * @brief Declares the handle a live tool holds to read closed periods while a session runs.
 * @author Mustafa Garip
 * @see modules/session/live/README.md
 */

#pragma once

#include <SushiProfiler/live/live_channel.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <memory>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief A reader of one session's closed periods.
     *
     * It shares the session's channel, so it stays valid after the session stops and after it
     * restarts; the next run's periods then replace the last run's. Each copy counts as a viewer,
     * so the consumer serves the view while any copy exists.
     */
    class SUSHIPROFILER_API LiveView final
    {
        public:
            /// @brief Builds a view of nothing: valid() is false and every read is empty.
            LiveView() noexcept = default;

            /// @brief Builds a view of @p channel and counts as a viewer of it.
            explicit LiveView(std::shared_ptr<LiveChannel> channel);

            LiveView(const LiveView& other);
            LiveView& operator=(const LiveView& other);
            LiveView(LiveView&& other) noexcept;
            LiveView& operator=(LiveView&& other) noexcept;
            ~LiveView();

            /// @brief Returns whether the view reads a channel.
            [[nodiscard]] bool valid() const noexcept;

            /// @brief Returns the newest closed period, or null before the first of the run.
            [[nodiscard]] std::shared_ptr<const ClosedPeriod> latest() const noexcept;

            /// @brief Returns the last @p count closed periods, oldest first.
            [[nodiscard]] std::vector<std::shared_ptr<const ClosedPeriod>> history(
                std::size_t count) const;

        private:
            std::shared_ptr<LiveChannel> channel_; /**< The channel read, or null. */
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
