/**************************************************************************/
/* lane_tracker.cpp                                                       */
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
 * @file lane_tracker.cpp
 * @brief Opens and closes lanes on lane events and answers the innermost open lane.
 * @author Mustafa Garip
 * @see modules/session/fold/README.md
 */

#include <SushiProfiler/fold/lane_tracker.hpp>

#include <algorithm>

namespace SushiProfiler
{
    NameID LaneTracker::lane_of(const TraceEvent& event)
    {
        const EventKind kind = kind_of(event);
        if (kind == EventKind::LaneBegin)
        {
            open_.push_back(event.name);
            return event.name;
        }
        if (kind == EventKind::LaneEnd)
        {
            const auto found = std::find(open_.rbegin(), open_.rend(), event.name);
            if (found == open_.rend())
            {
                ++unmatched_;
                return open_.empty() ? NameID{0U} : open_.back();
            }
            open_.erase(std::next(found).base());
            return event.name;
        }
        return open_.empty() ? NameID{0U} : open_.back();
    }

    std::size_t LaneTracker::open_lanes() const noexcept
    {
        return open_.size();
    }

    std::uint64_t LaneTracker::unmatched() const noexcept
    {
        return unmatched_;
    }

    void LaneTracker::reset() noexcept
    {
        open_.clear();
    }
} // namespace SushiProfiler
