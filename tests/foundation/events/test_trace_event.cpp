/**************************************************************************/
/* test_trace_event.cpp                                                   */
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
 * @file test_trace_event.cpp
 * @brief Tests the 24-byte trace event, its kind and site packing, and its value bits.
 * @author Mustafa Garip
 * @see tests/foundation/events/README.md
 */

#include <SushiProfiler/events/trace_event.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

using SushiProfiler::EventKind;
using SushiProfiler::K_EVENT_KIND_COUNT;
using SushiProfiler::K_MAX_SITE_ID;
using SushiProfiler::kind_of;
using SushiProfiler::pack_site;
using SushiProfiler::site_of;
using SushiProfiler::SiteID;
using SushiProfiler::TraceEvent;
using SushiProfiler::value_bits;
using SushiProfiler::value_of;

namespace
{
    /// @brief Returns the bit pattern of @p value.
    std::uint64_t bits_of(double value)
    {
        std::uint64_t bits = 0U;
        std::memcpy(&bits, &value, sizeof(bits));
        return bits;
    }
} // namespace

TEST(Small_TraceEvent, IsTwentyFourTriviallyCopyableBytes)
{
    EXPECT_EQ(sizeof(TraceEvent), 24U);
    EXPECT_TRUE(std::is_trivially_copyable_v<TraceEvent>);
}

TEST(Small_TraceEvent, KindAndSiteRoundTrip)
{
    for (std::uint8_t raw = 0U; raw < K_EVENT_KIND_COUNT; ++raw)
    {
        const auto kind = static_cast<EventKind>(raw);
        for (const SiteID site : {SiteID{1U}, K_MAX_SITE_ID})
        {
            TraceEvent event{};
            event.site_word = pack_site(kind, site);
            EXPECT_EQ(kind_of(event), kind);
            EXPECT_EQ(site_of(event), site);
        }
    }
}

TEST(Small_TraceEvent, ANaNValueKeepsItsBits)
{
    const std::uint64_t payload_nan = 0x7ff8000000001234ULL;
    double nan_value = 0.0;
    std::memcpy(&nan_value, &payload_nan, sizeof(nan_value));

    EXPECT_EQ(value_bits(nan_value), payload_nan);
    EXPECT_EQ(bits_of(value_of(payload_nan)), payload_nan);
    EXPECT_EQ(value_of(value_bits(std::numeric_limits<double>::infinity())),
              std::numeric_limits<double>::infinity());
    EXPECT_EQ(bits_of(value_of(value_bits(-0.0))), bits_of(-0.0));
}

TEST(Small_TraceEvent, ASpanLongerThanThirtyTwoBitsSurvives)
{
    TraceEvent event{};
    event.begin = 0U;
    event.end = 5'000'000'000ULL;

    EXPECT_EQ(event.end - event.begin, 5'000'000'000ULL);
}
