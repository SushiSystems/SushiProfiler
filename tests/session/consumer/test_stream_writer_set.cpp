/**************************************************************************/
/* test_stream_writer_set.cpp                                             */
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
 * @file test_stream_writer_set.cpp
 * @brief Tests that stream writers begin, write and end as requests and failures dictate.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/stream_writer_set.hpp>

#include <SushiProfiler/stream_writers/memory_stream_writer.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

using SushiProfiler::MemoryStreamWriter;
using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::StreamWriterSet;
using SushiProfiler::WriteStatus;

namespace
{
    /// @brief Returns a period of @p index.
    PeriodRecord period_of(std::uint64_t index)
    {
        PeriodRecord period;
        period.index = index;
        return period;
    }

    /// @brief Returns the index of each period @p writer kept, in the order written.
    std::vector<std::uint64_t> indices_of(const MemoryStreamWriter& writer)
    {
        std::vector<std::uint64_t> indices;
        for (const PeriodRecord& period : writer.periods())
            indices.push_back(period.index);
        return indices;
    }

    /// @brief Returns whether every index in @p indices is larger than the one before it.
    bool rises(const std::vector<std::uint64_t>& indices)
    {
        for (std::size_t index = 1U; index < indices.size(); ++index)
        {
            if (indices[index] <= indices[index - 1U])
                return false;
        }
        return true;
    }
    /// @brief Requests "trace" on and off alternately @p rounds times, then sets @p done.
    void toggle_requests(StreamWriterSet& set, int rounds, std::atomic<bool>& done)
    {
        bool enabled = false;
        for (int round = 0; round < rounds; ++round)
        {
            static_cast<void>(set.request("trace", enabled));
            enabled = !enabled;
        }
        done.store(true, std::memory_order_release);
    }

    /// @brief Applies requests and writes one period per turn, at least @p periods of them and
    /// until @p done is set.
    void write_until_done(StreamWriterSet& set, const RunRecord& record, std::uint64_t periods,
                          const std::atomic<bool>& done, std::vector<std::string>& problems)
    {
        for (std::uint64_t index = 0U; index < periods || !done.load(std::memory_order_acquire); ++index)
        {
            set.apply_requests(record, problems);
            set.write_period(record, period_of(index), {}, problems);
        }
    }
} // namespace

TEST(Small_StreamWriterSet, ADisabledWriterIsNotBegunUntilEnabled)
{
    MemoryStreamWriter writer;
    StreamWriterSet set({{"trace", &writer, false}});
    const RunRecord record;
    std::vector<std::string> problems;
    EXPECT_EQ(set.begin_enabled(record), "");
    set.write_period(record, period_of(0U), {}, problems);
    ASSERT_EQ(writer.begins(), 0U);

    ASSERT_TRUE(set.request("trace", true));
    set.apply_requests(record, problems);
    set.write_period(record, period_of(1U), {}, problems);

    EXPECT_EQ(writer.begins(), 1U);
    EXPECT_EQ(indices_of(writer), std::vector<std::uint64_t>{1U});
    EXPECT_TRUE(problems.empty());
}

TEST(Small_StreamWriterSet, ARequestAppliesOnlyInApplyRequests)
{
    MemoryStreamWriter writer;
    StreamWriterSet set({{"trace", &writer, true}});
    const RunRecord record;
    std::vector<std::string> problems;
    ASSERT_EQ(set.begin_enabled(record), "");

    ASSERT_TRUE(set.request("trace", false));
    set.write_period(record, period_of(0U), {}, problems);
    set.apply_requests(record, problems);
    set.write_period(record, period_of(1U), {}, problems);

    EXPECT_EQ(indices_of(writer), std::vector<std::uint64_t>{0U});
}

TEST(Small_StreamWriterSet, TurningOffStopsPeriodsButKeepsTheOutputOpenUntilEnd)
{
    MemoryStreamWriter writer;
    StreamWriterSet set({{"trace", &writer, true}});
    const RunRecord record;
    std::vector<std::string> problems;
    ASSERT_EQ(set.begin_enabled(record), "");
    set.write_period(record, period_of(0U), {}, problems);

    ASSERT_TRUE(set.request("trace", false));
    set.apply_requests(record, problems);
    set.write_period(record, period_of(1U), {}, problems);
    set.end_all(record, problems);

    EXPECT_EQ(indices_of(writer), std::vector<std::uint64_t>{0U});
    EXPECT_EQ(writer.ends(), 1U);
}

TEST(Small_StreamWriterSet, AWriterThatFailsToBeginWhenEnabledIsStoppedAndNamed)
{
    MemoryStreamWriter writer;
    writer.fail_with(WriteStatus::PathUnwritable);
    StreamWriterSet set({{"trace", &writer, false}});
    const RunRecord record;
    std::vector<std::string> problems;

    ASSERT_TRUE(set.request("trace", true));
    set.apply_requests(record, problems);
    set.write_period(record, period_of(0U), {}, problems);
    set.end_all(record, problems);

    ASSERT_EQ(problems.size(), 1U);
    EXPECT_EQ(problems[0], "writer \"trace\" did not start when enabled: its file could not be opened");
    EXPECT_EQ(writer.writes(), 0U);
    EXPECT_EQ(writer.ends(), 0U);
}

TEST(Small_StreamWriterSet, AFailedWriterIgnoresLaterRequests)
{
    MemoryStreamWriter writer;
    StreamWriterSet set({{"trace", &writer, true}});
    const RunRecord record;
    std::vector<std::string> problems;
    ASSERT_EQ(set.begin_enabled(record), "");
    writer.fail_with(WriteStatus::StreamFailed);
    set.write_period(record, period_of(0U), {}, problems);

    ASSERT_TRUE(set.request("trace", true));
    set.apply_requests(record, problems);
    set.write_period(record, period_of(1U), {}, problems);

    EXPECT_EQ(writer.writes(), 1U);
    ASSERT_EQ(problems.size(), 1U);
    EXPECT_EQ(problems[0], "writer \"trace\" failed on period 0 and was stopped");
}

TEST(Small_StreamWriterSet, EndAllEndsEveryBegunWriterOnceAndNoOther)
{
    MemoryStreamWriter begun;
    MemoryStreamWriter never;
    StreamWriterSet set({{"begun", &begun, true}, {"never", &never, false}});
    const RunRecord record;
    std::vector<std::string> problems;
    ASSERT_EQ(set.begin_enabled(record), "");

    set.end_all(record, problems);
    set.end_all(record, problems);

    EXPECT_EQ(begun.ends(), 1U);
    EXPECT_EQ(never.begins(), 0U);
    EXPECT_EQ(never.ends(), 0U);
}

TEST(Small_StreamWriterSet, AnUnknownNameIsRefused)
{
    MemoryStreamWriter writer;
    StreamWriterSet set({{"trace", &writer, true}});

    EXPECT_FALSE(set.request("other", true));
    EXPECT_TRUE(set.knows("trace"));
    EXPECT_FALSE(set.knows("other"));
}

TEST(Small_StreamWriterSet, BeginEnabledStopsAtTheFirstFailure)
{
    MemoryStreamWriter first;
    first.fail_with(WriteStatus::PathUnwritable);
    MemoryStreamWriter second;
    StreamWriterSet set({{"first", &first, true}, {"second", &second, true}});
    const RunRecord record;

    const std::string error = set.begin_enabled(record);

    EXPECT_EQ(error, "writer \"first\" did not start: its file could not be opened");
    EXPECT_EQ(second.begins(), 0U);
}

TEST(Small_StreamWriterSet, RequestsForOneWriterApplyInOrderAndTheLastOneWins)
{
    MemoryStreamWriter writer;
    StreamWriterSet set({{"trace", &writer, false}});
    const RunRecord record;
    std::vector<std::string> problems;
    ASSERT_EQ(set.begin_enabled(record), "");

    ASSERT_TRUE(set.request("trace", true));
    ASSERT_TRUE(set.request("trace", false));
    ASSERT_TRUE(set.request("trace", true));
    set.apply_requests(record, problems);
    set.write_period(record, period_of(0U), {}, problems);
    ASSERT_TRUE(set.request("trace", true));
    ASSERT_TRUE(set.request("trace", false));
    set.apply_requests(record, problems);
    set.write_period(record, period_of(1U), {}, problems);

    EXPECT_EQ(indices_of(writer), std::vector<std::uint64_t>{0U});
    EXPECT_EQ(writer.begins(), 1U);
    EXPECT_TRUE(problems.empty());
}

TEST(Medium_StreamWriterSet, RequestsFromAnotherThreadNeverTearAPeriod)
{
    MemoryStreamWriter writer;
    StreamWriterSet set({{"trace", &writer, true}});
    const RunRecord record;
    std::vector<std::string> problems;
    ASSERT_EQ(set.begin_enabled(record), "");
    std::atomic<bool> done{false};
    std::thread toggler(toggle_requests, std::ref(set), 1'000, std::ref(done));

    write_until_done(set, record, 1'000U, done, problems);
    toggler.join();
    set.apply_requests(record, problems);
    set.end_all(record, problems);

    EXPECT_TRUE(problems.empty());
    EXPECT_EQ(writer.begins(), 1U);
    EXPECT_EQ(writer.ends(), 1U);
    EXPECT_TRUE(rises(indices_of(writer)));
}
