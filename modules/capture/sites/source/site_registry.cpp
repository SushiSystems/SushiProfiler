/**************************************************************************/
/* site_registry.cpp                                                      */
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
 * @file site_registry.cpp
 * @brief Registers sites, copies their strings and keeps their enable bytes on the filter.
 * @author Mustafa Garip
 * @see modules/capture/sites/README.md
 */

#include <SushiProfiler/sites/site_registry.hpp>

#include <algorithm>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns @p text as a string, empty for null.
        std::string owned(const char* text)
        {
            return text != nullptr ? std::string(text) : std::string();
        }
    } // namespace

    SiteRegistry::SiteRegistry(std::size_t capacity)
        : capacity_(capacity), enabled_(new std::atomic<std::uint8_t>[capacity + 1U])
    {
        for (std::size_t index = 0U; index <= capacity_; ++index)
            enabled_[index].store(0U, std::memory_order_relaxed);
        infos_.reserve(std::min<std::size_t>(capacity_, 1024U));
    }

    SiteID SiteRegistry::register_site(SiteHandle& handle)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // Another thread may have registered the handle while this one waited for the lock.
        const SiteID cached = handle.id.load(std::memory_order_acquire);
        if (cached != K_UNREGISTERED)
            return cached == K_REFUSED ? 0U : cached;
        if (infos_.size() >= capacity_)
        {
            refused_.fetch_add(1U, std::memory_order_relaxed);
            handle.id.store(K_REFUSED, std::memory_order_release);
            return 0U;
        }
        const Site& site = *handle.site;
        SiteInfo info;
        info.name = owned(site.name);
        info.group = owned(site.group);
        info.file = owned(site.file);
        info.unit = owned(site.unit);
        info.line = site.line;
        info.level = site.level;
        info.role = site.role;
        info.op = site.op;
        const bool passes = passes_locked(info);
        infos_.push_back(std::move(info));
        const auto id = static_cast<SiteID>(infos_.size());
        enabled_[id].store(passes ? 1U : 0U, std::memory_order_relaxed);
        handle.id.store(id, std::memory_order_release);
        return id;
    }

    bool SiteRegistry::passes_locked(const SiteInfo& info) const
    {
        if (info.level > filter_.level)
            return false;
        if (filter_.groups.empty() || info.group.empty())
            return true;
        return std::find(filter_.groups.begin(), filter_.groups.end(), info.group) !=
               filter_.groups.end();
    }

    void SiteRegistry::apply(const SiteFilter& filter)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        filter_ = filter;
        for (std::size_t index = 0U; index < infos_.size(); ++index)
            enabled_[index + 1U].store(passes_locked(infos_[index]) ? 1U : 0U,
                                       std::memory_order_relaxed);
    }

    SiteInfo SiteRegistry::info(SiteID id) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (id == 0U || id > infos_.size())
            return SiteInfo{};
        return infos_[id - 1U];
    }

    std::size_t SiteRegistry::size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return infos_.size();
    }

    std::uint64_t SiteRegistry::refused() const noexcept
    {
        return refused_.load(std::memory_order_relaxed);
    }
} // namespace SushiProfiler
