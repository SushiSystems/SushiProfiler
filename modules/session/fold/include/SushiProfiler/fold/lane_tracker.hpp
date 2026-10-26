/**************************************************************************/
/* lane_tracker.hpp                                                       */
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
 * @file lane_tracker.hpp
 * @brief Declares the per-thread tracker that gives each event the lane open around it.
 * @author Mustafa Garip
 * @see modules/session/fold/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Follows one thread's lane events in ring order and names each event's lane.
     *
     * A lane begin opens its lane and a lane end closes the innermost open lane of that name.
     * An event between them belongs to the innermost open lane, or to lane 0 outside every lane.
     */
    class SUSHIPROFILER_API LaneTracker final
    {
        public:
            /// @brief Returns the lane of @p event, opening or closing a lane on lane events.
            [[nodiscard]] NameID lane_of(const TraceEvent& event);

            /// @brief Returns how many lanes are open.
            [[nodiscard]] std::size_t open_lanes() const noexcept;

            /// @brief Returns how many lane ends closed no open lane.
            [[nodiscard]] std::uint64_t unmatched() const noexcept;

            /// @brief Closes every lane, for when the gate was closed while lanes changed.
            void reset() noexcept;

        private:
            /// @brief The open lanes, innermost last.
            std::vector<NameID> open_;

            /// @brief Lane ends that closed no open lane.
            std::uint64_t unmatched_ = 0U;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
