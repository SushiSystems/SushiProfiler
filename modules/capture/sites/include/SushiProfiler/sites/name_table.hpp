/**************************************************************************/
/* name_table.hpp                                                         */
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
 * @file name_table.hpp
 * @brief Declares the table that interns run-time strings into ids.
 * @author Mustafa Garip
 * @see modules/capture/sites/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>

#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace SushiProfiler
{
    /**
     * @brief Gives each distinct string one id, from 1, and never frees it.
     *
     * Interning takes a lock and may allocate, so a hot path interns its names ahead of time
     * and records the ids.
     */
    class NameTable final
    {
        public:
            /// @brief Returns the id of @p text, interning it on first use; 0 for empty text.
            [[nodiscard]] NameID intern(std::string_view text);

            /// @brief Returns the text of @p id; empty for 0 or an id never given.
            [[nodiscard]] std::string text(NameID id) const;

            /// @brief Returns the number of interned strings.
            [[nodiscard]] std::size_t size() const;

        private:
            /// @brief Guards ids_ and texts_.
            mutable std::mutex mutex_;

            /// @brief The id of each interned text.
            std::unordered_map<std::string, NameID> ids_;

            /// @brief The text of id `index + 1`.
            std::vector<std::string> texts_;
    };
} // namespace SushiProfiler
