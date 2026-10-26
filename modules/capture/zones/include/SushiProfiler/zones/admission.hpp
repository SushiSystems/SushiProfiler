/**************************************************************************/
/* admission.hpp                                                          */
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
 * @file admission.hpp
 * @brief Declares the check every record makes before it writes: gate, site and ring.
 * @author Mustafa Garip
 * @see modules/capture/zones/README.md
 */

#pragma once

#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/rings/thread_ring.hpp>
#include <SushiProfiler/sites/site.hpp>
#include <SushiProfiler/sites/site_registry.hpp>

#include <new>
#include <system_error>

namespace SushiProfiler
{
    namespace Detail
    {
        /**
         * @brief Returns the calling thread's ring when @p handle's site records now, else null.
         * @param site Set to the site's id when the ring is returned.
         *
         * The gate is read first, so a closed collector costs one relaxed load. A site's first
         * use registers it, which may allocate; a failure there refuses the record.
         */
        [[nodiscard]] inline ThreadRing* admit(SiteHandle& handle, SiteID& site) noexcept
        {
            if (!Collector::recording())
                return nullptr;
            try
            {
                SiteRegistry& sites = Collector::instance().sites();
                site = sites.id_of(handle);
                if (!sites.enabled(site))
                    return nullptr;
            }
            catch (const std::bad_alloc&)
            {
                return nullptr;
            }
            catch (const std::system_error&)
            {
                return nullptr;
            }
            return Collector::ring();
        }
    } // namespace Detail
} // namespace SushiProfiler
