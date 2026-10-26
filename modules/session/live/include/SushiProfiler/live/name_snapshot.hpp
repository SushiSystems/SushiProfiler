/**************************************************************************/
/* name_snapshot.hpp                                                      */
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
 * @file name_snapshot.hpp
 * @brief Declares the immutable copy of a run's name tables a closed period carries.
 * @author Mustafa Garip
 * @see modules/session/live/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/version/export.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Holds the names, sites, tracks and thread names the ids of a closed period refer to.
     *
     * A reader resolves ids against the snapshot of its period, never against the collector, so
     * it needs no lock and never meets an id the snapshot lacks. The tables only grow, so a later
     * snapshot is a superset of an earlier one.
     */
    class SUSHIPROFILER_API NameSnapshot final
    {
        public:
            /// @brief Builds a snapshot from the tables; thread records keep their names only.
            NameSnapshot(std::vector<std::string> names, std::vector<SiteRecord> sites,
                         std::vector<std::string> tracks, const std::vector<ThreadRecord>& threads);

            /// @brief Returns the text of @p id, or an empty view for an id the snapshot lacks.
            [[nodiscard]] std::string_view name(NameID id) const noexcept;

            /// @brief Returns site @p id, or null for an id the snapshot lacks.
            [[nodiscard]] const SiteRecord* site(SiteID id) const noexcept;

            /// @brief Returns the name of track @p id, or an empty view.
            [[nodiscard]] std::string_view track(TrackID id) const noexcept;

            /// @brief Returns the name of thread @p index, or an empty view.
            [[nodiscard]] std::string_view thread_name(std::uint32_t index) const noexcept;

            /// @brief Returns every site, indexed by SiteID, for break_down_period().
            [[nodiscard]] const std::vector<SiteRecord>& sites() const noexcept;

            /// @brief Returns whether the tables and thread names of @p record are the snapshot's.
            [[nodiscard]] bool matches(const RunRecord& record) const;

        private:
            std::vector<std::string> names_;   /**< Names by NameID. */
            std::vector<SiteRecord> sites_;    /**< Sites by SiteID. */
            std::vector<std::string> tracks_;  /**< Tracks by TrackID. */
            /// @brief Thread index and name of each thread that has a name.
            std::vector<std::pair<std::uint32_t, std::string>> thread_names_;
    };

    /// @brief Makes a snapshot only when a table of the run grew since the last one.
    class SUSHIPROFILER_API NameSnapshotCache final
    {
        public:
            /// @brief Returns the last snapshot while the tables and names of @p record are as were.
            [[nodiscard]] std::shared_ptr<const NameSnapshot> snapshot(const RunRecord& record);

        private:
            std::shared_ptr<const NameSnapshot> last_; /**< The snapshot last returned. */
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
