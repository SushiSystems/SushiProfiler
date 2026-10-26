/**************************************************************************/
/* test_record_clock.cpp                                                  */
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
 * @file test_record_clock.cpp
 * @brief Tests moving ticks, steady times and each kind's time fields onto the record's clock.
 * @author Mustafa Garip
 * @see tests/session/fold/README.md
 */

#include <SushiProfiler/fold/record_clock.hpp>

#include <gtest/gtest.h>

using SushiProfiler::Calibration;
using SushiProfiler::CalibrationPoint;
using SushiProfiler::EventKind;
using SushiProfiler::RecordClock;
using SushiProfiler::TraceEvent;

namespace
{
    /// @brief Returns a clock of half a nanosecond per tick, tick 1000 at steady 5000, origin 5000.
    RecordClock tsc_clock()
    {
        const Calibration calibration(CalibrationPoint{1'000U, 5'000U},
                                      CalibrationPoint{3'000U, 6'000U});
        return RecordClock(calibration, 5'000U, true);
    }

    /// @brief Returns an event of @p kind with fields @p begin and @p end.
    TraceEvent event_of(EventKind kind, std::uint64_t begin, std::uint64_t end)
    {
        return TraceEvent{begin, end, SushiProfiler::pack_site(kind, 4U), 0U};
    }
} // namespace

TEST(Small_RecordClock, MapsTicksThroughTheCalibration)
{
    const RecordClock clock = tsc_clock();

    EXPECT_EQ(clock.from_ticks(2'000U), 500U);
    EXPECT_EQ(clock.from_ticks(1'000U), 0U);
    EXPECT_EQ(clock.from_steady(5'750U), 750U);
    EXPECT_EQ(clock.from_steady(4'000U), 0U);
}

TEST(Small_RecordClock, TakesSteadyTicksAsTheyAre)
{
    const RecordClock clock(Calibration(), 100U, false);

    EXPECT_EQ(clock.from_ticks(350U), 250U);
}

TEST(Small_RecordClock, ConvertsOnlyTheFieldsThatHoldTimes)
{
    const RecordClock clock = tsc_clock();

    const TraceEvent zone = clock.to_record(event_of(EventKind::Zone, 2'000U, 3'000U));
    const TraceEvent external = clock.to_record(event_of(EventKind::ExternalZone, 5'100U, 5'200U));
    const TraceEvent counter = clock.to_record(event_of(EventKind::Counter, 2'000U, 77U));

    EXPECT_EQ(zone.begin, 500U);
    EXPECT_EQ(zone.end, 1'000U);
    EXPECT_EQ(external.begin, 100U);
    EXPECT_EQ(external.end, 200U);
    EXPECT_EQ(counter.begin, 500U);
    EXPECT_EQ(counter.end, 77U);
    EXPECT_EQ(counter.site_word, SushiProfiler::pack_site(EventKind::Counter, 4U));
}
