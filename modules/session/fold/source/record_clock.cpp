/**************************************************************************/
/* record_clock.cpp                                                       */
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
 * @file record_clock.cpp
 * @brief Converts each kind's time fields, ticks or the caller's steady time, to record time.
 * @author Mustafa Garip
 * @see modules/session/fold/README.md
 */

#include <SushiProfiler/fold/record_clock.hpp>

namespace SushiProfiler
{
    RecordClock::RecordClock(const Calibration& calibration, std::uint64_t origin_steady_ns,
                             bool ticks_are_tsc) noexcept
        : calibration_(calibration), origin_steady_ns_(origin_steady_ns),
          ticks_are_tsc_(ticks_are_tsc)
    {

    }

    std::uint64_t RecordClock::from_ticks(std::uint64_t ticks) const noexcept
    {
        return from_steady(ticks_are_tsc_ ? calibration_.to_nanoseconds(ticks) : ticks);
    }

    std::uint64_t RecordClock::from_steady(std::uint64_t steady_ns) const noexcept
    {
        return steady_ns > origin_steady_ns_ ? steady_ns - origin_steady_ns_ : 0U;
    }

    TraceEvent RecordClock::to_record(const TraceEvent& event) const noexcept
    {
        TraceEvent converted = event;
        switch (kind_of(event))
        {
            case EventKind::Zone:
            case EventKind::Instant:
                converted.begin = from_ticks(event.begin);
                converted.end = from_ticks(event.end);
                break;
            case EventKind::ExternalZone:
            case EventKind::ExternalInstant:
                converted.begin = from_steady(event.begin);
                converted.end = from_steady(event.end);
                break;
            default:
                converted.begin = from_ticks(event.begin);
                break;
        }
        return converted;
    }
} // namespace SushiProfiler
