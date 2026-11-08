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

    SiteRegistry::SiteRegistry(std::size_t capacity, SiteCache cache)
        : capacity_(capacity), cache_(cache), enabled_(new std::atomic<std::uint8_t>[capacity + 1U])
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
        const SiteID id = add_locked(*handle.site);
        handle.id.store(id, std::memory_order_release);
        return id;
    }

    SiteID SiteRegistry::find_or_register(const SiteHandle& handle)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto found = ids_.find(handle.site);
        if (found != ids_.end())
            return found->second == K_REFUSED ? 0U : found->second;
        if (infos_.size() >= capacity_)
        {
            refused_.fetch_add(1U, std::memory_order_relaxed);
            ids_.emplace(handle.site, K_REFUSED);
            return 0U;
        }
        const SiteID id = add_locked(*handle.site);
        ids_.emplace(handle.site, id);
        return id;
    }

    SiteID SiteRegistry::add_locked(const Site& site)
    {
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
