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
    EXPECT_EQ(first->name(1U), "a");
    ASSERT_NE(first->site(1U), nullptr);
    EXPECT_EQ(first->site(1U)->group, "Model");
    EXPECT_EQ(first->track(0U), "gpu");
    EXPECT_EQ(first->thread_name(0U), "main");
}

TEST(Small_NameSnapshotCache, RebuildsWhenANameASiteATrackOrAThreadNameGrew)
{
    RunRecord record = small_record();
    NameSnapshotCache cache;
    auto previous = cache.snapshot(record);

    record.names.push_back("b");
    auto current = cache.snapshot(record);
    EXPECT_NE(current, previous);
    previous = current;

    record.sites.push_back(SiteRecord{});
    current = cache.snapshot(record);
    EXPECT_NE(current, previous);
    previous = current;

    record.tracks.push_back("copy");
    current = cache.snapshot(record);
    EXPECT_NE(current, previous);
    previous = current;

    record.threads[0].name = "render";
    current = cache.snapshot(record);
    EXPECT_NE(current, previous);
    EXPECT_EQ(current->thread_name(0U), "render");
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
