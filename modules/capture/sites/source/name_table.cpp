/**************************************************************************/
/* name_table.cpp                                                         */
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
 * @file name_table.cpp
 * @brief Interns run-time strings into ids under one lock.
 * @author Mustafa Garip
 * @see modules/capture/sites/README.md
 */

#include <SushiProfiler/sites/name_table.hpp>

namespace SushiProfiler
{
    NameID NameTable::intern(std::string_view text)
    {
        if (text.empty())
            return 0U;
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key(text);
        const auto found = ids_.find(key);
        if (found != ids_.end())
            return found->second;
        texts_.push_back(key);
        const auto id = static_cast<NameID>(texts_.size());
        ids_.emplace(std::move(key), id);
        return id;
    }

    std::string NameTable::text(NameID id) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (id == 0U || id > texts_.size())
            return std::string();
        return texts_[id - 1U];
    }

    std::size_t NameTable::size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return texts_.size();
    }
} // namespace SushiProfiler
