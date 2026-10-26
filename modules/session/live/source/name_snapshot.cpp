/**************************************************************************/
/* name_snapshot.cpp                                                      */
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
 * @file name_snapshot.cpp
 * @brief Copies a run's name tables and reuses the copy until one of them grows.
 * @author Mustafa Garip
 * @see modules/session/live/README.md
 */

#include <SushiProfiler/live/name_snapshot.hpp>

namespace SushiProfiler
{
    NameSnapshot::NameSnapshot(std::vector<std::string> names, std::vector<SiteRecord> sites,
                               std::vector<std::string> tracks,
                               const std::vector<ThreadRecord>& threads)
        : names_(std::move(names)), sites_(std::move(sites)), tracks_(std::move(tracks))
    {
        for (const ThreadRecord& thread : threads)
        {
            if (!thread.name.empty())
                thread_names_.emplace_back(thread.index, thread.name);
        }
    }

    std::string_view NameSnapshot::name(NameID id) const noexcept
    {
        return id < names_.size() ? std::string_view(names_[id]) : std::string_view();
    }

    const SiteRecord* NameSnapshot::site(SiteID id) const noexcept
    {
        return id < sites_.size() ? &sites_[id] : nullptr;
    }

    std::string_view NameSnapshot::track(TrackID id) const noexcept
    {
        return id < tracks_.size() ? std::string_view(tracks_[id]) : std::string_view();
    }

    std::string_view NameSnapshot::thread_name(std::uint32_t index) const noexcept
    {
        for (const auto& entry : thread_names_)
        {
            if (entry.first == index)
                return entry.second;
        }
        return std::string_view();
    }

    const std::vector<SiteRecord>& NameSnapshot::sites() const noexcept
    {
        return sites_;
    }

    bool NameSnapshot::matches(const RunRecord& record) const
    {
        if (record.names.size() != names_.size() || record.sites.size() != sites_.size() ||
            record.tracks.size() != tracks_.size())
            return false;
        std::size_t named = 0U;
        for (const ThreadRecord& thread : record.threads)
        {
            if (thread.name.empty())
                continue;
            if (thread_name(thread.index) != thread.name)
                return false;
            ++named;
        }
        return named == thread_names_.size();
    }

    std::shared_ptr<const NameSnapshot> NameSnapshotCache::snapshot(const RunRecord& record)
    {
        if (last_ != nullptr && last_->matches(record))
            return last_;
        last_ = std::make_shared<const NameSnapshot>(record.names, record.sites, record.tracks,
                                                     record.threads);
        return last_;
    }
} // namespace SushiProfiler
