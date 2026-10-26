/**************************************************************************/
/* test_drain_batch.cpp                                                   */
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
 * @file test_drain_batch.cpp
 * @brief Tests that an empty drain says nothing about any thread.
 * @author Mustafa Garip
 * @see tests/capture/collector/README.md
 */

#include <SushiProfiler/collector/drain_batch.hpp>

#include <gtest/gtest.h>

using SushiProfiler::DrainBatch;
using SushiProfiler::ThreadDrain;

TEST(Small_DrainBatch, AnEmptyDrainHoldsNoThreadAndNoDrop)
{
    const ThreadDrain thread;
    const DrainBatch batch;

    EXPECT_TRUE(thread.events.empty());
    EXPECT_EQ(thread.dropped, 0U);
    EXPECT_FALSE(thread.retired);
    EXPECT_TRUE(batch.threads.empty());
}
