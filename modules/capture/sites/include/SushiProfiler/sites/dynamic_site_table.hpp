/**************************************************************************/
/* dynamic_site_table.hpp                                                 */
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
 * @file dynamic_site_table.hpp
 * @brief Declares the table that gives each run-time counter or value name its own site.
 * @author Mustafa Garip
 * @see modules/capture/sites/README.md
 */

#pragma once

#include <SushiProfiler/sites/site.hpp>
#include <SushiProfiler/version/export.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Holds one site per distinct name built at run time, for one role.
     *
     * A static site names its measurement with a literal; a counter or value whose name is only
     * known at run time gets its site here on first use, so the record format needs no name
     * field on a sample. A lookup takes a shared lock and allocates nothing; adding a name takes
     * the unique lock and allocates, so it is not for a real-time thread.
     */
    class SUSHIPROFILER_API DynamicSiteTable final
    {
        public:
            /// @brief The most names a table holds; a later name is refused.
            static constexpr std::size_t K_MAX_NAMES = 4096U;

            /// @brief Builds an empty table whose sites record as @p role.
            explicit DynamicSiteTable(SiteRole role);

            DynamicSiteTable(const DynamicSiteTable&) = delete;
            DynamicSiteTable& operator=(const DynamicSiteTable&) = delete;
            DynamicSiteTable(DynamicSiteTable&&) = delete;
            DynamicSiteTable& operator=(DynamicSiteTable&&) = delete;
            ~DynamicSiteTable();

            /**
             * @brief Returns the handle of @p name, making it on first use; null when refused.
             *
             * The first call for a name fixes its unit, operation and level; later calls for the
             * same name get the same handle whatever they pass. The handle outlives the table's
             * users and never moves.
             */
            [[nodiscard]] SiteHandle* handle(std::string_view name, std::string_view unit,
                                             CounterOp op, std::uint8_t level);

            /// @brief Returns the number of names held.
            [[nodiscard]] std::size_t size() const;

            /// @brief Returns how many names were refused for want of room.
            [[nodiscard]] std::uint64_t refused() const noexcept;

        private:
            /// @brief One name's strings, site and handle, in one place that never moves.
            struct Entry;

            /// @brief What the sites of this table record.
            SiteRole role_;

            /// @brief Guards entries_.
            mutable std::shared_mutex mutex_;

            /// @brief The entries by name.
            std::map<std::string, std::unique_ptr<Entry>, std::less<>> entries_;

            /// @brief Names refused for want of room.
            std::atomic<std::uint64_t> refused_{0U};
    };

    /// @brief Returns the process-wide table of counter names built at run time.
    [[nodiscard]] SUSHIPROFILER_API DynamicSiteTable& dynamic_counter_sites();

    /// @brief Returns the process-wide table of value names built at run time.
    [[nodiscard]] SUSHIPROFILER_API DynamicSiteTable& dynamic_value_sites();
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
