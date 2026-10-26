/**************************************************************************/
/* site_registry.hpp                                                      */
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
 * @file site_registry.hpp
 * @brief Declares the registry that gives each site an id and holds the filter's enable bytes.
 * @author Mustafa Garip
 * @see modules/capture/sites/README.md
 */

#pragma once

#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/sites/site.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SushiProfiler
{
    /**
     * @brief Chooses the sites that record.
     *
     * A site records when its level is at most `level` and either `groups` is empty, the site
     * has no group, or its group is named here. An ungrouped site inherits its group from the
     * zone around it, which is not known until the events are folded.
     */
    struct SiteFilter
    {
        std::uint8_t level = 3U;         /**< The highest level that records. */
        std::vector<std::string> groups; /**< The groups that record; empty keeps all. */
    };

    /// @brief A registered site's description, with its strings owned.
    struct SiteInfo
    {
        std::string name;                /**< What the site measures. */
        std::string group;               /**< Its group, or empty. */
        std::string file;                /**< The source file. */
        std::string unit;                /**< A counter's or value's unit, or empty. */
        std::uint32_t line = 0U;         /**< The source line. */
        std::uint8_t level = 1U;         /**< Its density level. */
        SiteRole role = SiteRole::Zone;  /**< What it records. */
        CounterOp op = CounterOp::Add;   /**< A counter's operation. */
    };

    /**
     * @brief Gives each site one id on first use and answers, lock-free, whether it records.
     *
     * Ids run from 1 to the capacity. A site past the capacity is refused: its handle is marked
     * so it never takes the lock again, `id_of` returns 0, and `refused()` counts it once.
     */
    class SiteRegistry final
    {
        public:
            /// @brief The capacity of the process-wide registry.
            static constexpr std::size_t K_DEFAULT_SITE_CAPACITY = 65536U;

            /// @brief Builds an empty registry that holds up to @p capacity sites.
            explicit SiteRegistry(std::size_t capacity = K_DEFAULT_SITE_CAPACITY);

            SiteRegistry(const SiteRegistry&) = delete;
            SiteRegistry& operator=(const SiteRegistry&) = delete;
            SiteRegistry(SiteRegistry&&) = delete;
            SiteRegistry& operator=(SiteRegistry&&) = delete;
            ~SiteRegistry() = default;

            /// @brief Returns @p handle's id, registering its site on first use; 0 if refused.
            [[nodiscard]] SiteID id_of(SiteHandle& handle)
            {
                const SiteID cached = handle.id.load(std::memory_order_acquire);
                if (cached == K_UNREGISTERED)
                    return register_site(handle);
                return cached == K_REFUSED ? 0U : cached;
            }

            /// @brief Returns whether site @p id records under the current filter; false for 0.
            [[nodiscard]] bool enabled(SiteID id) const noexcept
            {
                return enabled_[id].load(std::memory_order_relaxed) != 0U;
            }

            /// @brief Applies @p filter to every site, and to every site registered later.
            void apply(const SiteFilter& filter);

            /// @brief Returns the description of site @p id; empty for an id never given.
            [[nodiscard]] SiteInfo info(SiteID id) const;

            /// @brief Returns the number of registered sites.
            [[nodiscard]] std::size_t size() const;

            /// @brief Returns the number of sites refused for want of capacity.
            [[nodiscard]] std::uint64_t refused() const noexcept;

        private:
            /// @brief A handle's id before its first use.
            static constexpr SiteID K_UNREGISTERED = 0U;

            /// @brief A handle's id once its site was refused.
            static constexpr SiteID K_REFUSED = ~SiteID{0U};

            /// @brief Registers @p handle's site under the lock, or finds it registered.
            SiteID register_site(SiteHandle& handle);

            /// @brief Returns whether @p info records under filter_; filter_ is read locked.
            [[nodiscard]] bool passes_locked(const SiteInfo& info) const;

            /// @brief The number of sites the registry holds at most.
            std::size_t capacity_;

            /// @brief One enable byte per id, 0 included, allocated once.
            std::unique_ptr<std::atomic<std::uint8_t>[]> enabled_;

            /// @brief Guards infos_ and filter_.
            mutable std::mutex mutex_;

            /// @brief The description of id `index + 1`.
            std::vector<SiteInfo> infos_;

            /// @brief The filter the enable bytes follow.
            SiteFilter filter_;

            /// @brief Sites refused for want of capacity.
            std::atomic<std::uint64_t> refused_{0U};
    };
} // namespace SushiProfiler
