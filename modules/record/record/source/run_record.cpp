/**************************************************************************/
/* run_record.cpp                                                         */
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
 * @file run_record.cpp
 * @brief Compares records field by field and looks up their names and sites.
 * @author Mustafa Garip
 * @see modules/record/record/README.md
 */

#include <SushiProfiler/record/run_record.hpp>

#include <tuple>

namespace SushiProfiler
{
    bool SiteRecord::operator==(const SiteRecord& other) const
    {
        return std::tie(name, group, file, unit, line, level, role, op) ==
               std::tie(other.name, other.group, other.file, other.unit, other.line, other.level,
                        other.role, other.op);
    }

    bool ThreadRecord::operator==(const ThreadRecord& other) const
    {
        return std::tie(index, name, dropped, late, unmatched) ==
               std::tie(other.index, other.name, other.dropped, other.late, other.unmatched);
    }

    bool ClockRecord::operator==(const ClockRecord& other) const
    {
        return std::tie(source, nanoseconds_per_tick, read_cost_ns) ==
               std::tie(other.source, other.nanoseconds_per_tick, other.read_cost_ns);
    }

    bool RecordDrops::operator==(const RecordDrops& other) const
    {
        return std::tie(outside_periods, open_at_stop, spans_dropped) ==
               std::tie(other.outside_periods, other.open_at_stop, other.spans_dropped);
    }

    std::string_view RunRecord::name_of(NameID name) const noexcept
    {
        return name < names.size() ? std::string_view(names[name]) : std::string_view();
    }

    const SiteRecord* RunRecord::site(SiteID site) const noexcept
    {
        return site != 0U && site < sites.size() ? &sites[site] : nullptr;
    }

    NameID RunRecord::find_name(std::string_view text) const noexcept
    {
        for (std::size_t index = 1U; index < names.size(); ++index)
        {
            if (names[index] == text)
                return static_cast<NameID>(index);
        }
        return 0U;
    }

    bool RunRecord::operator==(const RunRecord& other) const
    {
        return std::tie(environment, build, tags, clock, names, sites, tracks, threads, periods,
                        drops) ==
               std::tie(other.environment, other.build, other.tags, other.clock, other.names,
                        other.sites, other.tracks, other.threads, other.periods, other.drops);
    }
} // namespace SushiProfiler
