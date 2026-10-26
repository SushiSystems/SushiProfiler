/**************************************************************************/
/* test_live_view.cpp                                                     */
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
 * @file test_live_view.cpp
 * @brief Tests that a view reads its channel, counts as a viewer and outlives its owner.
 * @author Mustafa Garip
 * @see tests/session/live/README.md
 */

#include <SushiProfiler/live/live_view.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <utility>

using SushiProfiler::ClosedPeriod;
using SushiProfiler::LiveChannel;
using SushiProfiler::LiveView;

TEST(Small_LiveView, ADefaultViewIsInvalidAndReadsNothing)
{
    const LiveView view;

    EXPECT_FALSE(view.valid());
    EXPECT_EQ(view.latest(), nullptr);
    EXPECT_TRUE(view.history(5U).empty());
}

TEST(Small_LiveView, WatchedFollowsCopiesAndMoves)
{
    auto channel = std::make_shared<LiveChannel>(4U);
    {
        LiveView view(channel);
        EXPECT_TRUE(channel->watched());
        {
            const LiveView copy(view);
            LiveView moved(std::move(view));
            EXPECT_TRUE(channel->watched());
            EXPECT_FALSE(view.valid());
            EXPECT_TRUE(moved.valid());
        }
        EXPECT_FALSE(channel->watched());
    }
    EXPECT_FALSE(channel->watched());
}

TEST(Small_LiveView, ACopyAssignmentMovesTheViewerCount)
{
    auto channel = std::make_shared<LiveChannel>(4U);
    auto other = std::make_shared<LiveChannel>(4U);
    LiveView first(channel);
    LiveView second(other);

    second = first;

    EXPECT_TRUE(channel->watched());
    EXPECT_FALSE(other->watched());
}

TEST(Small_LiveView, AViewOutlivesItsChannelsOwner)
{
    auto channel = std::make_shared<LiveChannel>(4U);
    auto period = std::make_shared<ClosedPeriod>();
    period->sequence = 9U;
    channel->publish(period);
    LiveView view(channel);
    channel.reset();
    period.reset();

    ASSERT_NE(view.latest(), nullptr);
    EXPECT_EQ(view.latest()->sequence, 9U);
    EXPECT_EQ(view.history(3U).size(), 1U);
}
