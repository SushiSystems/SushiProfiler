/**************************************************************************/
/* test_stream_writer.cpp                                                 */
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
 * @file test_stream_writer.cpp
 * @brief Tests a stream writer driven through its interface: begin, each period in order, end.
 * @author Mustafa Garip
 * @see tests/session/stream_writers/README.md
 */

#include <SushiProfiler/stream_writers/stream_writer.hpp>

#include <gtest/gtest.h>

#include <string>

using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::StreamWriter;
using SushiProfiler::WriteStatus;
using SushiProfiler::ZoneInstance;

namespace
{
    /// @brief Notes each call as a word, and the period index and run count it was given.
    class NotingWriter final : public StreamWriter
    {
        public:
            /// @brief Notes `begin`.
            [[nodiscard]] WriteStatus begin(const RunRecord& record) override
            {
                static_cast<void>(record);
                calls += "begin ";
                return WriteStatus::Written;
            }

            /// @brief Notes the period's index and how many runs came with it.
            [[nodiscard]] WriteStatus write_period(
                const RunRecord& record, const PeriodRecord& period,
                const std::vector<ZoneInstance>& instances) override
            {
                static_cast<void>(record);
                calls += std::to_string(period.index) + ":" + std::to_string(instances.size()) +
                         " ";
                return WriteStatus::Written;
            }

            /// @brief Notes `end`.
            [[nodiscard]] WriteStatus end(const RunRecord& record) override
            {
                static_cast<void>(record);
                calls += "end";
                return WriteStatus::Written;
            }

            std::string calls; /**< The calls so far. */
    };
} // namespace

TEST(Small_StreamWriter, ReceivesBeginEachPeriodAndEnd)
{
    NotingWriter noting;
    StreamWriter& writer = noting;
    const RunRecord record;
    PeriodRecord first;
    first.index = 3U;
    PeriodRecord second;
    second.index = 4U;

    EXPECT_EQ(writer.begin(record), WriteStatus::Written);
    EXPECT_EQ(writer.write_period(record, first, {ZoneInstance{}}), WriteStatus::Written);
    EXPECT_EQ(writer.write_period(record, second, {}), WriteStatus::Written);
    EXPECT_EQ(writer.end(record), WriteStatus::Written);

    EXPECT_EQ(noting.calls, "begin 3:1 4:0 end");
}
