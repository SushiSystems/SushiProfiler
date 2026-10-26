/**************************************************************************/
/* named_records.hpp                                                      */
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
 * @file named_records.hpp
 * @brief Declares the counter and value records whose name is built at run time.
 * @author Mustafa Garip
 * @see modules/capture/zones/README.md
 */

#pragma once

#include <SushiProfiler/sites/dynamic_site_table.hpp>
#include <SushiProfiler/sites/site.hpp>
#include <SushiProfiler/zones/records.hpp>

#include <cstdint>
#include <new>
#include <string_view>
#include <system_error>

namespace SushiProfiler
{
    /**
     * @brief Records @p value at the counter named @p name, making its site on first use.
     *
     * Costs a shared lock and a map lookup beyond a static counter, and the first use of a name
     * allocates, so it is not for a real-time thread. A record that cannot allocate is dropped,
     * as a full ring drops. A table past its name limit drops the record too.
     */
    inline void record_counter_named(std::string_view name, std::string_view unit, CounterOp op,
                                     std::uint8_t level, double value) noexcept
    {
        try
        {
            SiteHandle* const handle = dynamic_counter_sites().handle(name, unit, op, level);
            if (handle != nullptr)
                record_counter(*handle, value);
        }
        catch (const std::bad_alloc&)
        {
            return;
        }
        catch (const std::system_error&)
        {
            return;
        }
    }

    /// @brief Records @p value at the value site named @p name; see record_counter_named().
    inline void record_value_named(std::string_view name, std::string_view unit,
                                   std::uint8_t level, double value) noexcept
    {
        try
        {
            SiteHandle* const handle =
                dynamic_value_sites().handle(name, unit, CounterOp::Add, level);
            if (handle != nullptr)
                record_value(*handle, value);
        }
        catch (const std::bad_alloc&)
        {
            return;
        }
        catch (const std::system_error&)
        {
            return;
        }
    }
} // namespace SushiProfiler
