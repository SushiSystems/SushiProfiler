/**************************************************************************/
/* live_view.cpp                                                          */
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
 * @file live_view.cpp
 * @brief Counts each view as a viewer of its channel and forwards reads to it.
 * @author Mustafa Garip
 * @see modules/session/live/README.md
 */

#include <SushiProfiler/live/live_view.hpp>

#include <utility>

namespace SushiProfiler
{
    LiveView::LiveView(std::shared_ptr<LiveChannel> channel) : channel_(std::move(channel))
    {
        if (channel_ != nullptr)
            channel_->attach();
    }

    LiveView::LiveView(const LiveView& other) : channel_(other.channel_)
    {
        if (channel_ != nullptr)
            channel_->attach();
    }

    LiveView& LiveView::operator=(const LiveView& other)
    {
        if (this == &other)
            return *this;
        if (other.channel_ != nullptr)
            other.channel_->attach();
        if (channel_ != nullptr)
            channel_->detach();
        channel_ = other.channel_;
        return *this;
    }

    LiveView::LiveView(LiveView&& other) noexcept : channel_(std::move(other.channel_))
    {
        other.channel_.reset();
    }

    LiveView& LiveView::operator=(LiveView&& other) noexcept
    {
        if (this == &other)
            return *this;
        if (channel_ != nullptr)
            channel_->detach();
        channel_ = std::move(other.channel_);
        other.channel_.reset();
        return *this;
    }

    LiveView::~LiveView()
    {
        if (channel_ != nullptr)
            channel_->detach();
    }

    bool LiveView::valid() const noexcept
    {
        return channel_ != nullptr;
    }

    std::shared_ptr<const ClosedPeriod> LiveView::latest() const noexcept
    {
        return channel_ != nullptr ? channel_->latest() : nullptr;
    }

    std::vector<std::shared_ptr<const ClosedPeriod>> LiveView::history(std::size_t count) const
    {
        if (channel_ == nullptr)
            return {};
        return channel_->history(count);
    }
} // namespace SushiProfiler
