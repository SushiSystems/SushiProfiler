/**************************************************************************/
/* period_publisher.cpp                                                   */
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
 * @file period_publisher.cpp
 * @brief Builds and publishes a closed period when a view is watching, and does nothing else.
 * @author Mustafa Garip
 * @see modules/session/live/README.md
 */

#include <SushiProfiler/live/period_publisher.hpp>

#include <utility>

namespace SushiProfiler
{
    PeriodPublisher::PeriodPublisher(LiveChannel& channel, const RunCap& cap,
                                     std::uint64_t origin_steady_ns) noexcept
        : channel_(channel), cap_(cap), origin_steady_ns_(origin_steady_ns)
    {

    }

    void PeriodPublisher::closed(const PeriodRecord& period, const RunRecord& record)
    {
        if (!channel_.watched())
            return;
        LiveCounts counts;
        for (const ThreadRecord& thread : record.threads)
        {
            counts.late += thread.late;
            counts.dropped += thread.dropped;
        }
        counts.spans_dropped = record.drops.spans_dropped;
        counts.outside = record.drops.outside_periods;
        channel_.publish(make_closed_period(period, channel_.run(), sequence_++,
                                            origin_steady_ns_, cap_, counts,
                                            names_.snapshot(record)));
    }
} // namespace SushiProfiler
