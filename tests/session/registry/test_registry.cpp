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
 * @file test_registry.cpp
 * @brief Tests lookups by name, the unknown-name refusals and the built-in writers' settings.
 * @author Mustafa Garip
 * @see tests/session/registry/README.md
 */

#include <SushiProfiler/registry/registry.hpp>

#include <gtest/gtest.h>

#include <sstream>

#include <SushiProfiler/json/json_reader.hpp>

using SushiProfiler::EnvironmentContributor;
using SushiProfiler::JsonValue;
using SushiProfiler::Made;
using SushiProfiler::RecordFacts;
using SushiProfiler::RecordWriter;
using SushiProfiler::Registry;
using SushiProfiler::Source;
using SushiProfiler::SourceContext;
using SushiProfiler::SourceStatus;
using SushiProfiler::SpanSink;
using SushiProfiler::StreamWriter;

namespace
{
    /// @brief Returns the value @p text holds.
    JsonValue parsed(const char* text)
    {
        return *SushiProfiler::read_json(text).value;
    }

    /// @brief A source that does nothing.
    class QuietSource final : public Source
    {
        public:
            /// @brief Returns `quiet`.
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "quiet";
            }

            /// @brief Starts.
            [[nodiscard]] SourceStatus begin(SourceContext& context) override
            {
                static_cast<void>(context);
                return SourceStatus{};
            }

            /// @brief Adds nothing.
            void collect(std::uint64_t period, SpanSink& sink) override
            {
                static_cast<void>(period);
                static_cast<void>(sink);
            }

            /// @brief Ends.
            void end() override
            {

            }
    };

    /// @brief Contributes nothing.
    class QuietContributor final : public EnvironmentContributor
    {
        public:
            /// @brief Adds no facts.
            void contribute(RecordFacts& facts) const override
            {
                static_cast<void>(facts);
            }
    };

    /// @brief Returns a registry with the built-in writers over @p terminal and source `quiet`.
    Registry registry_with_builtins(std::ostream& terminal)
    {
        Registry registry;
        SushiProfiler::register_builtin_writers(registry, terminal);
        const auto make_quiet = [](const JsonValue&)
        {
            return Made<Source>{std::make_unique<QuietSource>(), ""};
        };
        EXPECT_TRUE(registry.add_source("quiet", make_quiet));
        return registry;
    }
} // namespace

TEST(Small_Registry, MakesASourceByNameAndRefusesAnUnknownOne)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    const Made<Source> quiet = registry.make_source("quiet", parsed("{}"));
    const Made<Source> missing = registry.make_source("gpu", parsed("{}"));

    ASSERT_NE(quiet.part, nullptr);
    EXPECT_EQ(quiet.part->name(), "quiet");
    EXPECT_EQ(missing.part, nullptr);
    EXPECT_EQ(missing.error, "unknown source \"gpu\"; known: quiet");
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

TEST(Small_Registry, KnowsWhichWritersStreamAndListsEveryWriter)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    EXPECT_TRUE(registry.streams("chrome_trace"));
    EXPECT_FALSE(registry.streams("terminal"));
    EXPECT_EQ(registry.make_record_writer("chrome_trace", parsed("{}"), "").error,
              "unknown writer \"chrome_trace\"; known: chrome_trace, record, terminal");
    EXPECT_EQ(registry.make_stream_writer("csv", parsed("{}"), "").error,
              "unknown writer \"csv\"; known: chrome_trace, record, terminal");
}

TEST(Small_Registry, ReadsTheBuiltInWritersSettings)
{
    std::ostringstream terminal;
    const Registry registry = registry_with_builtins(terminal);

    EXPECT_NE(registry.make_record_writer("terminal", parsed(R"({"top": 5})"), "").part, nullptr);
    EXPECT_NE(registry.make_record_writer("terminal", parsed("{}"), "").part, nullptr);
    EXPECT_EQ(registry.make_record_writer("terminal", parsed(R"({"rows": 5})"), "").error,
              "writers.terminal: unknown key \"rows\"");
    EXPECT_NE(registry.make_record_writer("record", parsed(R"({"path": "a.json"})"), "out").part,
              nullptr);
    EXPECT_EQ(registry.make_record_writer("record", parsed("{}"), "out").error,
              "writers.record: missing key \"path\"");
    EXPECT_NE(registry.make_stream_writer("chrome_trace", parsed(R"({"path": "t.json"})"), "")
                  .part,
              nullptr);
}

TEST(Small_Registry, KeepsContributorsInOrder)
{
    Registry registry;
    const QuietContributor first;
    const QuietContributor second;

    registry.add_contributor(first);
    registry.add_contributor(second);

    ASSERT_EQ(registry.contributors().size(), 2U);
    EXPECT_EQ(registry.contributors()[0], &first);
    EXPECT_EQ(registry.contributors()[1], &second);
}
