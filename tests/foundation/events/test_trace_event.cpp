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
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
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
#include <tuple>

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

class Small_TraceEventPacking
    : public ::testing::TestWithParam<std::tuple<std::uint8_t, SiteID>>
{
};

TEST_P(Small_TraceEventPacking, AKindAndASiteSurviveInTheSiteWord)
{
    const auto kind = static_cast<EventKind>(std::get<0>(GetParam()));
    const SiteID site = std::get<1>(GetParam());
    TraceEvent event{};

    event.site_word = pack_site(kind, site);

    EXPECT_EQ(kind_of(event), kind);
    EXPECT_EQ(site_of(event), site);
}

INSTANTIATE_TEST_SUITE_P(Small_Sites, Small_TraceEventPacking,
                         ::testing::Combine(::testing::Range<std::uint8_t>(0U, K_EVENT_KIND_COUNT),
                                            ::testing::Values(SiteID{1U}, K_MAX_SITE_ID)));

TEST(Small_TraceEvent, ANaNWithAPayloadKeepsItsBits)
{
    const std::uint64_t payload_nan = 0x7ff8000000001234ULL;
    double nan_value = 0.0;
    std::memcpy(&nan_value, &payload_nan, sizeof(nan_value));

    EXPECT_EQ(value_bits(nan_value), payload_nan);
    EXPECT_EQ(bits_of(value_of(payload_nan)), payload_nan);
}

TEST(Small_TraceEvent, InfinityKeepsItsValue)
{
    constexpr double infinity = std::numeric_limits<double>::infinity();

    EXPECT_EQ(value_of(value_bits(infinity)), infinity);
}

TEST(Small_TraceEvent, NegativeZeroKeepsItsSign)
{
    EXPECT_EQ(bits_of(value_of(value_bits(-0.0))), bits_of(-0.0));
}
