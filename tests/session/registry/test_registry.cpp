/**************************************************************************/
/* test_registry.cpp                                                      */
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
 * @file test_registry.cpp
 * @brief Tests lookups by name, the unknown-name refusals and the built-in writers' settings.
 * @author Mustafa Garip
 * @see tests/session/registry/README.md
 */

#include <SushiProfiler/registry/registry.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <SushiProfiler/environment/memory_contributor.hpp>
#include <SushiProfiler/environment/record_facts.hpp>
#include <SushiProfiler/json/json_reader.hpp>
#include <SushiProfiler/record/span_table.hpp>
#include <SushiProfiler/sites/name_table.hpp>
#include <SushiProfiler/sources/memory_source.hpp>

using SushiProfiler::EnvironmentContributor;
using SushiProfiler::JsonValue;
using SushiProfiler::Made;
using SushiProfiler::MemoryContributor;
using SushiProfiler::MemorySource;
using SushiProfiler::NameTable;
using SushiProfiler::RecordFacts;
using SushiProfiler::RecordWriter;
using SushiProfiler::Registry;
using SushiProfiler::PeriodSpan;
using SushiProfiler::Source;
using SushiProfiler::SourceContext;
using SushiProfiler::SourceStatus;
using SushiProfiler::Span;
using SushiProfiler::SpanSink;
using SushiProfiler::SpanTable;
using SushiProfiler::StreamWriter;

namespace
{
    /// @brief Returns the value @p text holds.
    JsonValue parsed(const char* text)
    {
        return *SushiProfiler::read_json(text).value;
    }

    /// @brief The work a fake device library has queued, shared with its source.
    class FakeDeviceQueue final
    {
        public:
            /// @brief Queues one piece of work that started at steady time @p start_ns.
            void push(std::uint64_t start_ns)
            {
                const std::lock_guard<std::mutex> lock(mutex_);
                starts_.push_back(start_ns);
            }

            /// @brief Returns and forgets the work queued so far.
            [[nodiscard]] std::vector<std::uint64_t> take()
            {
                const std::lock_guard<std::mutex> lock(mutex_);
                return std::exchange(starts_, {});
            }

        private:
            std::mutex mutex_;                  /**< Guards starts_ against the library threads. */
            std::vector<std::uint64_t> starts_; /**< The steady start of each piece of work. */
    };

    /// @brief The fake library's source: hands each period the work that started inside it.
    class FakeDeviceSource final : public Source
    {
        public:
            /// @brief Builds a source that reads @p queue, which must outlive it.
            explicit FakeDeviceSource(FakeDeviceQueue& queue) : queue_(queue)
            {

            }

            /// @brief Returns `fake_device`.
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "fake_device";
            }

            /// @brief Adds the track `fake_device.queue` and the span name `kernel`.
            [[nodiscard]] SourceStatus begin(SourceContext& context) override
            {
                track_ = context.add_track("fake_device.queue");
                kernel_ = context.intern("kernel");
                return SourceStatus{};
            }

            /// @brief Adds the work inside @p period, drops older work as late, keeps newer work.
            void collect(const PeriodSpan& period, SpanSink& sink) override
            {
                for (const std::uint64_t start : queue_.take())
                    pending_.push_back(start);
                for (auto start = pending_.begin(); start != pending_.end();)
                {
                    if (*start >= period.end_steady_ns)
                    {
                        ++start;
                        continue;
                    }
                    if (*start < period.start_steady_ns)
                    {
                        sink.drop_late(1U);
                    }
                    else
                    {
                        Span span;
                        span.queued_ns = *start;
                        span.start_ns = *start;
                        span.end_ns = *start + 10U;
                        span.track = track_;
                        span.name = kernel_;
                        static_cast<void>(sink.append(span));
                    }
                    start = pending_.erase(start);
                }
            }

            /// @brief Ends.
            void end() override
            {

            }

        private:
            FakeDeviceQueue& queue_;                /**< The library's queue. */
            std::vector<std::uint64_t> pending_;    /**< Work taken but not yet placed. */
            std::uint32_t track_ = 0U;              /**< The device track. */
            SushiProfiler::NameID kernel_ = 0U;     /**< The span name. */
    };

    /// @brief The fake library's contribution to the record's environment.
    class FakeDeviceContributor final : public EnvironmentContributor
    {
        public:
            /// @brief Sets the fact `fake_device` to the library's version.
            void contribute(RecordFacts& facts) const override
            {
                SushiProfiler::set_fact(facts.environment, "fake_device", "1.0");
            }
    };

    /// @brief What the fake library registers: it owns the queue and the contributor.
    class FakeDeviceProfilerParts final
    {
        public:
            /// @brief Registers the source `fake_device` and the contributor in @p registry.
            explicit FakeDeviceProfilerParts(Registry& registry)
            {
                const auto make_source = [this](const JsonValue& settings)
                {
                    static_cast<void>(settings);
                    return Made<Source>{std::make_unique<FakeDeviceSource>(queue_), ""};
                };
                registry.add_source("fake_device", make_source);
                registry.add_contributor(contributor_);
            }

            FakeDeviceProfilerParts(const FakeDeviceProfilerParts&) = delete;
            FakeDeviceProfilerParts& operator=(const FakeDeviceProfilerParts&) = delete;
            FakeDeviceProfilerParts(FakeDeviceProfilerParts&&) = delete;
            FakeDeviceProfilerParts& operator=(FakeDeviceProfilerParts&&) = delete;
            ~FakeDeviceProfilerParts() = default;

            /// @brief Returns the queue the library pushes its work to.
            [[nodiscard]] FakeDeviceQueue& queue() noexcept
            {
                return queue_;
            }

        private:
            FakeDeviceQueue queue_;               /**< The work the source reads. */
            FakeDeviceContributor contributor_;   /**< Held by the registry by reference. */
    };

    /// @brief Registers the fake library's parts in @p registry and returns what owns them.
    [[nodiscard]] FakeDeviceProfilerParts register_profiler_parts(Registry& registry)
    {
        return FakeDeviceProfilerParts(registry);
    }

    /// @brief Returns a registry with the built-in writers over @p terminal and source `quiet`.
    Registry registry_with_builtins(std::ostream& terminal)
    {
        Registry registry;
        SushiProfiler::register_builtin_writers(registry, terminal);
        const auto make_quiet = [](const JsonValue&)
        {
            return Made<Source>{std::make_unique<MemorySource>("quiet"), ""};
        };
        EXPECT_TRUE(registry.add_source("quiet", make_quiet));
        return registry;
    }
} // namespace

TEST(Small_Registry, MakesASourceByName)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    const Made<Source> quiet = registry.make_source("quiet", parsed("{}"));

    ASSERT_NE(quiet.part, nullptr);
    EXPECT_EQ(quiet.part->name(), "quiet");
}

TEST(Small_Registry, RefusesAnUnknownSourceAndListsTheKnownOnes)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    const Made<Source> missing = registry.make_source("gpu", parsed("{}"));

    EXPECT_EQ(missing.part, nullptr);
    EXPECT_EQ(missing.error, "unknown source \"gpu\"; known: quiet");
}

namespace
{
    /// @brief Holds a registry with the fake device library's parts and what a source needs.
    class Small_RegistryDevice : public ::testing::Test
    {
        protected:
            /// @brief Makes the source, queues work at 50, 150 and 250 ns and collects the periods
            /// [100, 200) and [200, 300).
            void collect_two_periods()
            {
                Made<Source> made = registry_.make_source("fake_device", parsed("{}"));
                ASSERT_NE(made.part, nullptr);
                ASSERT_TRUE(made.part->begin(context_).ok());
                parts_.queue().push(50U);
                parts_.queue().push(150U);
                parts_.queue().push(250U);
                made.part->collect(PeriodSpan{0U, 100U, 200U}, sink_);
                made.part->collect(PeriodSpan{1U, 200U, 300U}, sink_);
                made.part->end();
            }

            Registry registry_;                                           /**< The registry. */
            FakeDeviceProfilerParts parts_ = register_profiler_parts(registry_); /**< Its parts. */
            NameTable names_;                                             /**< The names. */
            std::vector<std::string> tracks_;                             /**< The tracks. */
            SourceContext context_{names_, tracks_, 0U};                  /**< The context. */
            SpanTable table_;                                             /**< The spans. */
            SpanSink sink_{table_, names_, tracks_};                      /**< Their sink. */
    };
} // namespace

TEST_F(Small_RegistryDevice, TheRegisteredSourcePlacesWorkInThePeriodHoldingItsStart)
{
    collect_two_periods();

    ASSERT_EQ(table_.size(), 2U);
    EXPECT_EQ(table_.span(0U).start_ns, 150U);
    EXPECT_EQ(table_.span(1U).start_ns, 250U);
}

TEST_F(Small_RegistryDevice, TheRegisteredSourceCountsOlderWorkAsLate)
{
    collect_two_periods();

    EXPECT_EQ(sink_.late(), 1U);
}

TEST_F(Small_RegistryDevice, TheRegisteredSourceAddsItsTrack)
{
    collect_two_periods();

    EXPECT_EQ(tracks_, std::vector<std::string>{"fake_device.queue"});
}

TEST_F(Small_RegistryDevice, TheRegisteredContributorsFactReachesTheGatheredFacts)
{
    const SushiProfiler::RecordFacts facts = SushiProfiler::gather_facts(registry_.contributors());

    ASSERT_FALSE(facts.environment.empty());
    EXPECT_EQ(facts.environment.back().name, "fake_device");
    EXPECT_EQ(facts.environment.back().value, "1.0");
}

TEST(Small_Registry, ANameIsTakenOnce)
{
    std::ostringstream terminal;
    Registry registry = registry_with_builtins(terminal);
    const auto make_none = [](const JsonValue&, const std::string&)
    {
        return Made<RecordWriter>{};
    };

    EXPECT_FALSE(registry.add_record_writer("terminal", make_none));
    EXPECT_FALSE(registry.add_record_writer("chrome_trace", make_none));
}

TEST(Small_Registry, KnowsWhichWritersStream)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    EXPECT_TRUE(registry.streams("chrome_trace"));
    EXPECT_FALSE(registry.streams("terminal"));
}

TEST(Small_Registry, AnUnknownWriterListsEveryWriter)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    EXPECT_EQ(registry.make_record_writer("chrome_trace", parsed("{}"), "").error,
              "unknown writer \"chrome_trace\"; known: chrome_trace, record, terminal");
    EXPECT_EQ(registry.make_stream_writer("csv", parsed("{}"), "").error,
              "unknown writer \"csv\"; known: chrome_trace, record, terminal");
}

TEST(Small_Registry, TheTerminalWriterAcceptsTopOrNoSettings)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    EXPECT_NE(registry.make_record_writer("terminal", parsed(R"({"top": 5})"), "").part, nullptr);
    EXPECT_NE(registry.make_record_writer("terminal", parsed("{}"), "").part, nullptr);
}

TEST(Small_Registry, TheTerminalWriterRefusesAnUnknownKey)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    EXPECT_EQ(registry.make_record_writer("terminal", parsed(R"({"rows": 5})"), "").error,
              "writers.terminal: unknown key \"rows\"");
}

TEST(Small_Registry, TheRecordWriterAcceptsAPath)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    EXPECT_NE(registry.make_record_writer("record", parsed(R"({"path": "a.json"})"), "out").part,
              nullptr);
}

TEST(Small_Registry, TheRecordWriterRequiresAPath)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    EXPECT_EQ(registry.make_record_writer("record", parsed("{}"), "out").error,
              "writers.record: missing key \"path\"");
}

TEST(Small_Registry, TheChromeTraceWriterAcceptsAPath)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    EXPECT_NE(registry.make_stream_writer("chrome_trace", parsed(R"({"path": "t.json"})"), "")
                  .part,
              nullptr);
}

TEST(Small_Registry, KeepsContributorsInOrder)
{
    Registry registry;
    const MemoryContributor first;
    const MemoryContributor second;

    registry.add_contributor(first);
    registry.add_contributor(second);

    ASSERT_EQ(registry.contributors().size(), 2U);
    EXPECT_EQ(registry.contributors()[0], &first);
    EXPECT_EQ(registry.contributors()[1], &second);
}
