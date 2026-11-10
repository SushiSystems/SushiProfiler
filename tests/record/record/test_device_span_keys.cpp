/**************************************************************************/
/* test_device_span_keys.cpp                                              */
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
 * @file test_device_span_keys.cpp
 * @brief Tests the shared span attribute keys and the ceiling fact key builders.
 * @author Mustafa Garip
 * @see tests/record/record/README.md
 */

#include <SushiProfiler/record/device_span_keys.hpp>

#include <SushiProfiler/analysis/roofline_analysis.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <string>

namespace Keys = SushiProfiler::DeviceSpanKeys;

using SushiProfiler::Cell;
using SushiProfiler::PeriodRecord;
using SushiProfiler::Row;
using SushiProfiler::RunRecord;
using SushiProfiler::Span;
using SushiProfiler::Table;

namespace
{
    /// @brief Returns a record of one `gemm` span of 100 us on `queue 0`, written with the keys of
    /// the header, over the ceilings 1e12 FLOP/s and 1e11 B/s.
    ///
    /// The span carries 1e8 FLOP and 1e6 B. When @p labelled, it also carries node 3, type `f16`,
    /// graph `g` and origin `o`, and a ceiling of 4e12 FLOP/s for `f16`.
    RunRecord written_record(bool labelled)
    {
        RunRecord record;
        record.names = {"",
                        "gemm",
                        std::string(Keys::K_FLOPS),
                        std::string(Keys::K_BYTES),
                        std::string(Keys::K_NODE),
                        std::string(Keys::K_DTYPE),
                        std::string(Keys::K_GRAPH),
                        std::string(Keys::K_ORIGIN),
                        "f16",
                        "g",
                        "o"};
        record.tracks = {"", "queue 0"};
        record.environment = {{Keys::peak_flops_fact("queue 0"), "1e12"},
                              {Keys::peak_bytes_per_s_fact("queue 0"), "1e11"},
                              {Keys::peak_flops_fact("queue 0", "f16"), "4e12"}};
        PeriodRecord period;
        period.index = 1U;
        period.timeline = 1U;
        period.end_ns = 1'000'000U;
        const std::size_t row = period.spans.append(Span{0U, 0U, 100'000U, 1U, 1U, 0U, 0U});
        period.spans.set_integer(row, 2U, 100'000'000);
        period.spans.set_integer(row, 3U, 1'000'000);
        if (labelled)
        {
            period.spans.set_integer(row, 4U, 3);
            period.spans.set_name(row, 5U, 8U);
            period.spans.set_name(row, 6U, 9U);
            period.spans.set_name(row, 7U, 10U);
        }
        record.periods = {period};
        return record;
    }
} // namespace

TEST(Small_DeviceSpanKeys, TheCostKeysAndTheCeilingFactsAreReadByTheRoofline)
{
    const Table table = SushiProfiler::RooflineAnalysis().run(written_record(false), {});

    ASSERT_EQ(table.rows.size(), 1U);
    const Row& row = table.rows[0];
    EXPECT_EQ(row[table.column("flops")], Cell::whole(100'000'000));
    EXPECT_EQ(row[table.column("bytes")], Cell::whole(1'000'000));
    EXPECT_EQ(row[table.column("peak_flops")], Cell::real(1e12));
    EXPECT_EQ(row[table.column("peak_bytes_per_s")], Cell::real(1e11));
    EXPECT_EQ(row[table.column("limit")], Cell::text("compute"));
}

TEST(Small_DeviceSpanKeys, TheLabelKeysAndThePerTypeCeilingAreReadByTheRoofline)
{
    const Table table = SushiProfiler::RooflineAnalysis().run(written_record(true), {});

    ASSERT_EQ(table.rows.size(), 1U);
    const Row& row = table.rows[0];
    EXPECT_EQ(row[table.column("node")], Cell::whole(3));
    EXPECT_EQ(row[table.column("dtype")], Cell::text("f16"));
    EXPECT_EQ(row[table.column("graph")], Cell::text("g"));
    EXPECT_EQ(row[table.column("origin")], Cell::text("o"));
    EXPECT_EQ(row[table.column("peak_flops")], Cell::real(4e12));
}

TEST(Small_DeviceSpanKeys, TheCeilingFactsNameTheTrack)
{
    EXPECT_EQ(Keys::peak_flops_fact("queue 0"), "device.queue 0.peak_flops");
    EXPECT_EQ(Keys::peak_bytes_per_s_fact("queue 0"), "device.queue 0.peak_bytes_per_s");
}

TEST(Small_DeviceSpanKeys, ThePerTypeComputeCeilingAppendsTheType)
{
    EXPECT_EQ(Keys::peak_flops_fact("queue 0", "f16"), "device.queue 0.peak_flops.f16");
}

TEST(Small_DeviceSpanKeys, AnEmptyTrackLeavesTheSeparatorsInPlace)
{
    EXPECT_EQ(Keys::peak_flops_fact(""), "device..peak_flops");
}
