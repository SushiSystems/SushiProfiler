/**************************************************************************/
/* test_name_snapshot.cpp                                                 */
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
 * @file test_name_snapshot.cpp
 * @brief Tests that a snapshot resolves ids on its own and is rebuilt only when a table grew.
 * @author Mustafa Garip
 * @see tests/session/live/README.md
 */

#include <SushiProfiler/live/name_snapshot.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <string>

using SushiProfiler::NameSnapshot;
using SushiProfiler::NameSnapshotCache;
using SushiProfiler::RunRecord;
using SushiProfiler::SiteRecord;
using SushiProfiler::ThreadRecord;

namespace
{
    /// @brief Returns a record with names `a`, one site named `zone`, track `gpu` and thread 0.
    RunRecord small_record()
    {
        RunRecord record;
        record.names = {"", "a"};
        record.sites.resize(2U);
        record.sites[1].name = "zone";
        record.sites[1].group = "Model";
        record.tracks = {"gpu"};
        ThreadRecord thread;
        thread.index = 0U;
        thread.name = "main";
        record.threads.push_back(thread);
        return record;
    }
} // namespace

TEST(Small_NameSnapshotCache, ReusesTheSnapshotWhileTablesAreUnchanged)
{
    const RunRecord record = small_record();
    NameSnapshotCache cache;

    const auto first = cache.snapshot(record);
    const auto second = cache.snapshot(record);

    EXPECT_EQ(first, second);
}

TEST(Small_NameSnapshotCache, ASnapshotResolvesTheIdsOfItsRecord)
{
    const RunRecord record = small_record();
    NameSnapshotCache cache;

    const auto snapshot = cache.snapshot(record);

    EXPECT_EQ(snapshot->name(1U), "a");
    ASSERT_NE(snapshot->site(1U), nullptr);
    EXPECT_EQ(snapshot->site(1U)->group, "Model");
    EXPECT_EQ(snapshot->track(0U), "gpu");
    EXPECT_EQ(snapshot->thread_name(0U), "main");
}

namespace
{
    /// @brief One change to a record that a snapshot must notice.
    struct ChangeRow
    {
        void (*change)(RunRecord&);
    };

    /// @brief Adds a name.
    void grow_names(RunRecord& record)
    {
        record.names.push_back("b");
    }

    /// @brief Adds a site.
    void grow_sites(RunRecord& record)
    {
        record.sites.push_back(SiteRecord{});
    }

    /// @brief Adds a track.
    void grow_tracks(RunRecord& record)
    {
        record.tracks.push_back("copy");
    }

    /// @brief Renames thread 0.
    void rename_thread(RunRecord& record)
    {
        record.threads[0].name = "render";
    }
} // namespace

class Small_NameSnapshotChange : public ::testing::TestWithParam<ChangeRow>
{
};

TEST_P(Small_NameSnapshotChange, TheCacheRebuildsWhenATableOrAThreadNameChanged)
{
    RunRecord record = small_record();
    NameSnapshotCache cache;
    const auto previous = cache.snapshot(record);

    GetParam().change(record);

    EXPECT_NE(cache.snapshot(record), previous);
}

TEST_P(Small_NameSnapshotChange, ASnapshotDoesNotMatchARecordThatChanged)
{
    RunRecord record = small_record();
    NameSnapshotCache cache;
    const auto snapshot = cache.snapshot(record);

    GetParam().change(record);

    EXPECT_FALSE(snapshot->matches(record));
}

INSTANTIATE_TEST_SUITE_P(Small_Tables, Small_NameSnapshotChange,
                         ::testing::Values(ChangeRow{grow_names}, ChangeRow{grow_sites},
                                           ChangeRow{grow_tracks}, ChangeRow{rename_thread}));

TEST(Small_NameSnapshotCache, ARenamedThreadShowsItsNewNameInTheNextSnapshot)
{
    RunRecord record = small_record();
    NameSnapshotCache cache;
    static_cast<void>(cache.snapshot(record));

    rename_thread(record);

    EXPECT_EQ(cache.snapshot(record)->thread_name(0U), "render");
}

TEST(Small_NameSnapshotCache, ASnapshotMatchesTheRecordItCopied)
{
    const RunRecord record = small_record();
    NameSnapshotCache cache;

    EXPECT_TRUE(cache.snapshot(record)->matches(record));
}

TEST(Small_NameSnapshotCache, SitesReturnsEverySiteIndexedBySiteId)
{
    const RunRecord record = small_record();
    NameSnapshotCache cache;

    const auto snapshot = cache.snapshot(record);

    ASSERT_EQ(snapshot->sites().size(), 2U);
    EXPECT_EQ(snapshot->sites()[1].name, "zone");
    EXPECT_EQ(snapshot->sites()[1].group, "Model");
}

TEST(Small_NameSnapshotCache, ASnapshotKeepsItsTextAfterTheRecordGrows)
{
    RunRecord record = small_record();
    NameSnapshotCache cache;
    const auto before = cache.snapshot(record);

    record.names[1] = "changed";
    record.names.push_back("b");
    record.sites[1].name = "other";

    EXPECT_EQ(before->name(1U), "a");
    EXPECT_EQ(before->name(2U), "");
    EXPECT_EQ(before->site(1U)->name, "zone");
}

TEST(Small_NameSnapshotCache, UnknownIdsResolveToNothing)
{
    const RunRecord record = small_record();
    NameSnapshotCache cache;
    const auto snapshot = cache.snapshot(record);

    EXPECT_EQ(snapshot->name(99U), "");
    EXPECT_EQ(snapshot->site(99U), nullptr);
    EXPECT_EQ(snapshot->track(99U), "");
    EXPECT_EQ(snapshot->thread_name(99U), "");
}
