/**************************************************************************/
/* test_record_tool.cpp                                                   */
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
 * @file test_record_tool.cpp
 * @brief Tests each record tool command, its output and its exit code, on files it writes.
 * @author Mustafa Garip
 * @see tests/tools/record_tool/README.md
 */

#include <SushiProfiler/record_tool/record_tool.hpp>

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

#include <SushiProfiler/record_json/record_json.hpp>

using SushiProfiler::K_TOOL_FAILED;
using SushiProfiler::K_TOOL_OK;
using SushiProfiler::K_TOOL_USAGE;
using SushiProfiler::PeriodRecord;
using SushiProfiler::RunRecord;

namespace
{
    /// @brief What one run of the tool printed and returned.
    struct ToolRun
    {
        int code = 0;    /**< The exit code. */
        std::string out; /**< What went to the output. */
        std::string err; /**< What went to the error stream. */
    };

    /// @brief Runs the tool on @p arguments.
    ToolRun run(const std::vector<std::string>& arguments)
    {
        std::ostringstream out;
        std::ostringstream err;
        ToolRun result;
        result.code = SushiProfiler::run_record_tool(arguments, out, err);
        result.out = out.str();
        result.err = err.str();
        return result;
    }

    /// @brief Writes @p text to the temporary file @p name and returns its path.
    std::string temporary_file(const std::string& name, const std::string& text)
    {
        const std::string path = ::testing::TempDir() + name;
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file << text;
        return path;
    }

    /// @brief Returns a record of timeline `step` whose four periods each last @p length ns.
    RunRecord steps_of(std::uint64_t length, const std::string& build_type = "release")
    {
        RunRecord record;
        record.names = {"", "step"};
        record.sites = {SushiProfiler::SiteRecord{}};
        record.build = {{"build_type", build_type}};
        record.threads = {SushiProfiler::ThreadRecord{0U, "main", 0U, 0U, 0U}};
        for (std::uint64_t index = 0U; index < 4U; ++index)
        {
            PeriodRecord period;
            period.index = index;
            period.timeline = 1U;
            period.begin_ns = index * 1'000U;
            period.end_ns = period.begin_ns + length;
            record.periods.push_back(period);
        }
        return record;
    }

    /// @brief Writes @p record to the temporary file @p name and returns its path.
    std::string record_file(const std::string& name, const RunRecord& record)
    {
        return temporary_file(name, SushiProfiler::write_record(record));
    }
} // namespace

TEST(Medium_RecordTool, ValidateAcceptsAGoodRecordAndNamesItsPeriodsAndThreads)
{
    const std::string good = record_file("tool_validate_good.sprec.json", steps_of(100U));

    const ToolRun valid = run({"validate", good});

    EXPECT_EQ(valid.code, K_TOOL_OK) << valid.err;
    EXPECT_EQ(valid.out, good + ": valid, 4 periods, 1 threads\n");
}

TEST(Medium_RecordTool, ValidateFailsOnAnUnsupportedFormatVersion)
{
    const std::string bad =
        temporary_file("tool_validate_bad.sprec.json", R"({"sushiprofiler_record": 9})");

    const ToolRun invalid = run({"validate", bad});

    EXPECT_EQ(invalid.code, K_TOOL_FAILED);
    EXPECT_EQ(invalid.err.rfind(bad + ": ", 0U), 0U);
}

TEST(Medium_RecordTool, ValidateRefusesAMissingFileWithTheUsageCode)
{
    const std::string none = ::testing::TempDir() + "tool_validate_none.sprec.json";

    const ToolRun missing = run({"validate", none});

    EXPECT_EQ(missing.code, K_TOOL_USAGE);
    EXPECT_NE(missing.err.find("could not be read"), std::string::npos);
}

TEST(Medium_RecordTool, ShowPrintsOnlyTheNamedAnalysis)
{
    const std::string path = record_file("tool_show_named.sprec.json", steps_of(100U));

    const ToolRun shown = run({"show", path, "--analysis", "distribution", "--top", "3"});

    EXPECT_EQ(shown.code, K_TOOL_OK);
    EXPECT_NE(shown.out.find("== distribution =="), std::string::npos);
    EXPECT_EQ(shown.out.find("== breakdown =="), std::string::npos);
}

TEST(Medium_RecordTool, ShowPrintsTheDefaultAnalysesWhenNoneIsNamed)
{
    const std::string path = record_file("tool_show_default.sprec.json", steps_of(100U));

    const ToolRun defaults = run({"show", path});

    EXPECT_EQ(defaults.code, K_TOOL_OK);
    EXPECT_NE(defaults.out.find("== overhead =="), std::string::npos);
}

TEST(Medium_RecordTool, ShowRefusesAnUnknownAnalysis)
{
    const std::string path = record_file("tool_show_unknown.sprec.json", steps_of(100U));

    const ToolRun unknown = run({"show", path, "--analysis", "fastest"});

    EXPECT_EQ(unknown.code, K_TOOL_USAGE);
    EXPECT_NE(unknown.err.find("unknown analysis \"fastest\""), std::string::npos);
}

TEST(Medium_RecordTool, ShowRefusesATopThatIsNotANumber)
{
    const std::string path = record_file("tool_show_top.sprec.json", steps_of(100U));

    const ToolRun bad_top = run({"show", path, "--top", "zero"});

    EXPECT_EQ(bad_top.code, K_TOOL_USAGE);
}

TEST(Medium_RecordTool, CompareNamesTheFasterRecord)
{
    const std::string baseline = record_file("tool_compare_base.sprec.json", steps_of(100U));
    const std::string faster = record_file("tool_compare_fast.sprec.json", steps_of(50U));

    const ToolRun compared = run({"compare", baseline, faster});

    EXPECT_EQ(compared.code, K_TOOL_OK);
    EXPECT_NE(compared.out.find("== compare =="), std::string::npos);
    EXPECT_NE(compared.out.find("faster"), std::string::npos);
}

TEST(Medium_RecordTool, CompareRefusesRecordsOfDifferentBuilds)
{
    const std::string baseline = record_file("tool_refuse_base.sprec.json", steps_of(100U));
    const std::string debug = record_file("tool_refuse_debug.sprec.json", steps_of(50U, "debug"));

    const ToolRun refused = run({"compare", baseline, debug});

    EXPECT_EQ(refused.code, K_TOOL_FAILED);
    EXPECT_NE(refused.err.find("the builds differ at \"build_type\""), std::string::npos);
}

TEST(Medium_RecordTool, CompareComparesDifferentBuildsWhenTheMismatchIsAllowed)
{
    const std::string baseline = record_file("tool_allow_base.sprec.json", steps_of(100U));
    const std::string debug = record_file("tool_allow_debug.sprec.json", steps_of(50U, "debug"));

    const ToolRun allowed = run({"compare", baseline, debug, "--allow-build-mismatch"});

    EXPECT_EQ(allowed.code, K_TOOL_OK);
}

TEST(Medium_RecordTool, CheckPrintsPassForACheckThatHolds)
{
    const std::string path = record_file("tool_check_pass.sprec.json", steps_of(100U));
    const std::string passing = temporary_file(
        "tool_check_pass.json",
        R"([{"subject": "timeline:step", "statistic": "median", "at_most_ns": 150}])");

    const ToolRun passed = run({"check", path, passing});

    EXPECT_EQ(passed.code, K_TOOL_OK);
    EXPECT_EQ(passed.out, "pass  timeline:step median = 100\n1 of 1 checks passed\n");
}

TEST(Medium_RecordTool, CheckFailsWhenOneCheckFails)
{
    const std::string path = record_file("tool_check_fail.sprec.json", steps_of(100U));
    const std::string failing = temporary_file(
        "tool_check_fail.json",
        R"([{"subject": "timeline:step", "statistic": "max", "at_most_ns": 90},
            {"subject": "timeline:step", "statistic": "min", "at_least_ns": 50}])");

    const ToolRun failed = run({"check", path, failing});

    EXPECT_EQ(failed.code, K_TOOL_FAILED);
    EXPECT_EQ(failed.out, "FAIL  timeline:step: max is 100 ns, above at most 90 ns\n"
                          "pass  timeline:step min = 100\n1 of 2 checks passed\n");
}

TEST(Medium_RecordTool, CheckRefusesAFileOfChecksThatDoesNotParse)
{
    const std::string path = record_file("tool_check_broken.sprec.json", steps_of(100U));
    const std::string broken = temporary_file("tool_check_broken.json", R"([{"subject": 1}])");

    const ToolRun refused = run({"check", path, broken});

    EXPECT_EQ(refused.code, K_TOOL_USAGE);
    EXPECT_EQ(refused.err, broken + ": checks[0].subject: a string was expected\n");
}

TEST(Small_RecordTool, NoCommandPrintsTheUsageAndExitsWithTheUsageCode)
{
    const ToolRun empty = run({});

    EXPECT_EQ(empty.code, K_TOOL_USAGE);
    EXPECT_EQ(empty.err.rfind("usage: sp_record", 0U), 0U);
}

TEST(Small_RecordTool, HelpPrintsTheUsageAndSucceeds)
{
    const ToolRun help = run({"help"});

    EXPECT_EQ(help.code, K_TOOL_OK);
    EXPECT_EQ(help.out.rfind("usage: sp_record", 0U), 0U);
}

TEST(Small_RecordTool, AnUnknownCommandIsNamed)
{
    const ToolRun unknown = run({"plot"});

    EXPECT_EQ(unknown.code, K_TOOL_USAGE);
    EXPECT_EQ(unknown.err.rfind("unknown command \"plot\"", 0U), 0U);
}

TEST(Small_RecordTool, AnUnknownOptionIsNamed)
{
    const ToolRun option = run({"validate", "a.json", "--fast"});

    EXPECT_EQ(option.err, "unknown option --fast\n");
}

TEST(Small_RecordTool, AnOptionWithoutItsValueIsNamed)
{
    const ToolRun missing_value = run({"show", "a.json", "--top"});

    EXPECT_EQ(missing_value.err, "--top needs a value\n");
}
