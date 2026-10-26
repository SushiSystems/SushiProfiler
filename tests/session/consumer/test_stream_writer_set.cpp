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
 * @file test_stream_writer_set.cpp
 * @brief Tests that stream writers begin, write and end as requests and failures dictate.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/stream_writer_set.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;
using SushiProfiler::StreamWriter;
using SushiProfiler::StreamWriterSet;
using SushiProfiler::WriteStatus;
using SushiProfiler::ZoneRun;

namespace
{
    /// @brief A writer that logs its calls and can fail to begin or to write.
    class LoggingWriter final : public StreamWriter
    {
        public:
            /// @brief Logs `begin` and fails when told to.
            [[nodiscard]] WriteStatus begin(const RunRecord& record) override
            {
                static_cast<void>(record);
                calls += "b";
                return fail_begin ? WriteStatus::PathUnwritable : WriteStatus::Written;
            }

            /// @brief Logs `w` and the period index; fails when told to.
            [[nodiscard]] WriteStatus write_period(const RunRecord& record,
                                                   const PeriodRecord& period,
                                                   const std::vector<ZoneRun>& runs) override
            {
                static_cast<void>(record);
                static_cast<void>(runs);
                calls += "w" + std::to_string(period.index);
                return fail_write ? WriteStatus::StreamFailed : WriteStatus::Written;
            }

            /// @brief Logs `e`.
            [[nodiscard]] WriteStatus end(const RunRecord& record) override
            {
                static_cast<void>(record);
                calls += "e";
                return WriteStatus::Written;
            }

            std::string calls;          /**< The calls so far, one letter or letter and index each. */
            bool fail_begin = false;    /**< Whether begin fails. */
            bool fail_write = false;    /**< Whether write_period fails. */
    };

    /// @brief Returns a period of @p index.
    PeriodRecord period_of(std::uint64_t index)
    {
        PeriodRecord period;
        period.index = index;
        return period;
    }
} // namespace

TEST(Small_StreamWriterSet, ADisabledWriterIsNotBegunUntilEnabled)
{
    LoggingWriter writer;
    StreamWriterSet set({{"trace", &writer, false}});
    const RunRecord record;
    std::vector<std::string> problems;

    EXPECT_EQ(set.begin_enabled(record), "");
    set.write_period(record, period_of(0U), {}, problems);
    EXPECT_EQ(writer.calls, "");

    ASSERT_TRUE(set.request("trace", true));
    set.apply_requests(record, problems);
    set.write_period(record, period_of(1U), {}, problems);

    EXPECT_EQ(writer.calls, "bw1");
    EXPECT_TRUE(problems.empty());
}

TEST(Small_StreamWriterSet, ARequestAppliesOnlyInApplyRequests)
{
    LoggingWriter writer;
    StreamWriterSet set({{"trace", &writer, true}});
    const RunRecord record;
    std::vector<std::string> problems;
    ASSERT_EQ(set.begin_enabled(record), "");

    ASSERT_TRUE(set.request("trace", false));
    set.write_period(record, period_of(0U), {}, problems);
    set.apply_requests(record, problems);
    set.write_period(record, period_of(1U), {}, problems);

    EXPECT_EQ(writer.calls, "bw0");
}

TEST(Small_StreamWriterSet, TurningOffStopsPeriodsButKeepsTheOutputOpenUntilEnd)
{
    LoggingWriter writer;
    StreamWriterSet set({{"trace", &writer, true}});
    const RunRecord record;
    std::vector<std::string> problems;
    ASSERT_EQ(set.begin_enabled(record), "");
    set.write_period(record, period_of(0U), {}, problems);

    ASSERT_TRUE(set.request("trace", false));
    set.apply_requests(record, problems);
    set.write_period(record, period_of(1U), {}, problems);
    set.end_all(record, problems);

    EXPECT_EQ(writer.calls, "bw0e");
}

TEST(Small_StreamWriterSet, AWriterThatFailsToBeginWhenEnabledIsStoppedAndNamed)
{
    LoggingWriter writer;
    writer.fail_begin = true;
    StreamWriterSet set({{"trace", &writer, false}});
    const RunRecord record;
    std::vector<std::string> problems;

    ASSERT_TRUE(set.request("trace", true));
    set.apply_requests(record, problems);
    set.write_period(record, period_of(0U), {}, problems);
    set.end_all(record, problems);

    ASSERT_EQ(problems.size(), 1U);
    EXPECT_EQ(problems[0], "writer \"trace\" did not start when enabled: its file could not be opened");
    EXPECT_EQ(writer.calls, "b");
}

TEST(Small_StreamWriterSet, AFailedWriterIgnoresLaterRequests)
{
    LoggingWriter writer;
    writer.fail_write = true;
    StreamWriterSet set({{"trace", &writer, true}});
    const RunRecord record;
    std::vector<std::string> problems;
    ASSERT_EQ(set.begin_enabled(record), "");
    set.write_period(record, period_of(0U), {}, problems);

    ASSERT_TRUE(set.request("trace", true));
    set.apply_requests(record, problems);
    set.write_period(record, period_of(1U), {}, problems);

    EXPECT_EQ(writer.calls, "bw0");
    ASSERT_EQ(problems.size(), 1U);
    EXPECT_EQ(problems[0], "writer \"trace\" failed on period 0 and was stopped");
}

TEST(Small_StreamWriterSet, EndAllEndsEveryBegunWriterOnceAndNoOther)
{
    LoggingWriter begun;
    LoggingWriter never;
    StreamWriterSet set({{"begun", &begun, true}, {"never", &never, false}});
    const RunRecord record;
    std::vector<std::string> problems;
    ASSERT_EQ(set.begin_enabled(record), "");

    set.end_all(record, problems);
    set.end_all(record, problems);

    EXPECT_EQ(begun.calls, "be");
    EXPECT_EQ(never.calls, "");
}

TEST(Small_StreamWriterSet, AnUnknownNameIsRefused)
{
    LoggingWriter writer;
    StreamWriterSet set({{"trace", &writer, true}});

    EXPECT_FALSE(set.request("other", true));
    EXPECT_TRUE(set.knows("trace"));
    EXPECT_FALSE(set.knows("other"));
}

TEST(Small_StreamWriterSet, BeginEnabledStopsAtTheFirstFailure)
{
    LoggingWriter first;
    first.fail_begin = true;
    LoggingWriter second;
    StreamWriterSet set({{"first", &first, true}, {"second", &second, true}});
    const RunRecord record;

    const std::string error = set.begin_enabled(record);

    EXPECT_EQ(error, "writer \"first\" did not start: its file could not be opened");
    EXPECT_EQ(second.calls, "");
}

TEST(Medium_StreamWriterSet, RequestsFromAnotherThreadNeverTearAPeriod)
{
    LoggingWriter writer;
    StreamWriterSet set({{"trace", &writer, true}});
    const RunRecord record;
    std::vector<std::string> problems;
    ASSERT_EQ(set.begin_enabled(record), "");
    std::atomic<bool> done{false};
    std::thread toggler(
        [&]
        {
            bool enabled = false;
            for (int round = 0; round < 1'000; ++round)
            {
                static_cast<void>(set.request("trace", enabled));
                enabled = !enabled;
            }
            done.store(true, std::memory_order_release);
        });
    for (std::uint64_t index = 0U; index < 1'000U || !done.load(std::memory_order_acquire); ++index)
    {
        set.apply_requests(record, problems);
        set.write_period(record, period_of(index), {}, problems);
    }
    toggler.join();
    set.apply_requests(record, problems);
    set.end_all(record, problems);

    EXPECT_TRUE(problems.empty());
    ASSERT_FALSE(writer.calls.empty());
    EXPECT_EQ(writer.calls.front(), 'b');
    EXPECT_EQ(writer.calls.back(), 'e');
    EXPECT_EQ(writer.calls.find('b', 1U), std::string::npos);
    std::size_t last_period = 0U;
    std::size_t position = 1U;
    bool first = true;
    while ((position = writer.calls.find('w', position)) != std::string::npos)
    {
        const std::size_t digits = writer.calls.find_first_not_of("0123456789", position + 1U);
        const std::size_t period =
            std::stoull(writer.calls.substr(position + 1U, digits - position - 1U));
        if (!first)
            EXPECT_GT(period, last_period);
        last_period = period;
        first = false;
        position = digits;
    }
}
