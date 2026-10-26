/**************************************************************************/
/* dynamic_site_table.cpp                                                 */
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
 * @file dynamic_site_table.cpp
 * @brief Keeps one stable site per run-time name behind a shared lock, up to a fixed count.
 * @author Mustafa Garip
 * @see modules/capture/sites/README.md
 */

#include <SushiProfiler/sites/dynamic_site_table.hpp>

#include <mutex>
#include <utility>

namespace SushiProfiler
{
    struct DynamicSiteTable::Entry
    {
        /// @brief Copies the strings and builds the site over them.
        Entry(std::string_view name_text, std::string_view unit_text, SiteRole role, CounterOp op,
              std::uint8_t level)
            : name(name_text), unit(unit_text),
              site{name.c_str(), nullptr, "(run-time name)", 0U, level, role, op,
                   unit.empty() ? nullptr : unit.c_str()},
              handle{&site}
        {

        }

        std::string name;   /**< The site's name. */
        std::string unit;   /**< The site's unit, or empty. */
        Site site;          /**< The static description over the strings above. */
        SiteHandle handle;  /**< The handle records go through. */
    };

    DynamicSiteTable::DynamicSiteTable(SiteRole role) : role_(role)
    {

    }

    DynamicSiteTable::~DynamicSiteTable() = default;

    SiteHandle* DynamicSiteTable::handle(std::string_view name, std::string_view unit,
                                         CounterOp op, std::uint8_t level)
    {
        {
            const std::shared_lock<std::shared_mutex> lock(mutex_);
            const auto found = entries_.find(name);
            if (found != entries_.end())
                return &found->second->handle;
        }
        const std::unique_lock<std::shared_mutex> lock(mutex_);
        const auto found = entries_.find(name);
        if (found != entries_.end())
            return &found->second->handle;
        if (entries_.size() >= K_MAX_NAMES)
        {
            refused_.fetch_add(1U, std::memory_order_relaxed);
            return nullptr;
        }
        auto entry = std::make_unique<Entry>(name, unit, role_, op, level);
        SiteHandle* const made = &entry->handle;
        entries_.emplace(std::string(name), std::move(entry));
        return made;
    }

    std::size_t DynamicSiteTable::size() const
    {
        const std::shared_lock<std::shared_mutex> lock(mutex_);
        return entries_.size();
    }

    std::uint64_t DynamicSiteTable::refused() const noexcept
    {
        return refused_.load(std::memory_order_relaxed);
    }

    DynamicSiteTable& dynamic_counter_sites()
    {
        // Never destroyed: a thread that records after main returns still finds it.
        static DynamicSiteTable* const table = new DynamicSiteTable(SiteRole::Counter);
        return *table;
    }

    DynamicSiteTable& dynamic_value_sites()
    {
        static DynamicSiteTable* const table = new DynamicSiteTable(SiteRole::Value);
        return *table;
    }
} // namespace SushiProfiler
