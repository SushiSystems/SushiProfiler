/**************************************************************************/
/* pause_log.cpp                                                          */
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
 * @file pause_log.cpp
 * @brief Keeps the pause intervals under one mutex and answers overlap queries.
 * @author Mustafa Garip
 * @see modules/session/consumer/README.md
 */

#include <SushiProfiler/consumer/pause_log.hpp>

#include <algorithm>
#include <limits>

namespace SushiProfiler
{
    void PauseLog::begin(std::uint64_t steady_ns)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (open_)
            return;
        open_ = true;
        open_begin_ns_ = steady_ns;
    }

    void PauseLog::end(std::uint64_t steady_ns)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (!open_)
            return;
        open_ = false;
        closed_.push_back(Interval{open_begin_ns_, std::max(steady_ns, open_begin_ns_)});
        if (closed_.size() > K_MAX_INTERVALS)
            closed_.erase(closed_.begin());
    }

    bool PauseLog::paused() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return open_;
    }

    bool PauseLog::overlaps(std::uint64_t begin_steady_ns, std::uint64_t end_steady_ns) const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (open_ && end_steady_ns > open_begin_ns_)
            return true;
        for (const Interval& interval : closed_)
        {
            if (begin_steady_ns < interval.end_ns && end_steady_ns > interval.begin_ns)
                return true;
        }
        return false;
    }

    std::vector<std::uint64_t> PauseLog::ends_after(std::uint64_t steady_ns) const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::uint64_t> ends;
        for (const Interval& interval : closed_)
        {
            if (interval.end_ns > steady_ns)
                ends.push_back(interval.end_ns);
        }
        return ends;
    }

    void PauseLog::forget_before(std::uint64_t steady_ns)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        closed_.erase(std::remove_if(closed_.begin(), closed_.end(),
                                     [steady_ns](const Interval& interval)
                                     {
                                         return interval.end_ns < steady_ns;
                                     }),
                      closed_.end());
    }
} // namespace SushiProfiler
