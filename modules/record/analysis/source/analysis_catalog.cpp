/**************************************************************************/
/* analysis_catalog.cpp                                                   */
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
 * @file analysis_catalog.cpp
 * @brief Keeps analysis factories sorted by name and makes them on request.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/analysis_catalog.hpp>

#include <algorithm>

namespace SushiProfiler
{
    namespace
    {
        /// @brief Orders an entry before a name.
        struct EntryBefore
        {
            /// @brief Returns whether @p entry sorts before @p name.
            bool operator()(const std::pair<std::string, AnalysisFactory>& entry,
                            std::string_view name) const noexcept
            {
                return entry.first < name;
            }
        };
    } // namespace

    bool AnalysisCatalog::add(std::string name, AnalysisFactory factory)
    {
        const auto at = std::lower_bound(entries_.begin(), entries_.end(), name, EntryBefore{});
        if (at != entries_.end() && at->first == name)
            return false;
        entries_.emplace(at, std::move(name), factory);
        return true;
    }

    std::unique_ptr<Analysis> AnalysisCatalog::make(std::string_view name) const
    {
        const auto at = std::lower_bound(entries_.begin(), entries_.end(), name, EntryBefore{});
        if (at == entries_.end() || at->first != name)
            return nullptr;
        return at->second();
    }

    bool AnalysisCatalog::knows(std::string_view name) const noexcept
    {
        const auto at = std::lower_bound(entries_.begin(), entries_.end(), name, EntryBefore{});
        return at != entries_.end() && at->first == name;
    }

    std::vector<std::string> AnalysisCatalog::names() const
    {
        std::vector<std::string> names;
        names.reserve(entries_.size());
        for (const auto& entry : entries_)
            names.push_back(entry.first);
        return names;
    }
} // namespace SushiProfiler
