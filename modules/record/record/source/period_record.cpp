/**************************************************************************/
/* period_record.cpp                                                      */
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
 * @file period_record.cpp
 * @brief Compares periods, their trees, tags and facts field by field.
 * @author Mustafa Garip
 * @see modules/record/record/README.md
 */

#include <SushiProfiler/record/period_record.hpp>

#include <tuple>

namespace SushiProfiler
{
    bool Fact::operator==(const Fact& other) const
    {
        return name == other.name && value == other.value;
    }

    bool Tag::operator==(const Tag& other) const
    {
        return key == other.key && value == other.value;
    }

    bool ZoneNode::operator==(const ZoneNode& other) const
    {
        return std::tie(site, name, parent, inclusive_ns, self_ns, calls) ==
               std::tie(other.site, other.name, other.parent, other.inclusive_ns, other.self_ns,
                        other.calls);
    }

    bool ZoneTree::operator==(const ZoneTree& other) const
    {
        return thread == other.thread && lane == other.lane && nodes == other.nodes;
    }

    bool ZoneRun::operator==(const ZoneRun& other) const
    {
        return std::tie(thread, lane, site, name, begin_ns, end_ns, depth) ==
               std::tie(other.thread, other.lane, other.site, other.name, other.begin_ns,
                        other.end_ns, other.depth);
    }

    bool PeriodRecord::operator==(const PeriodRecord& other) const
    {
        return std::tie(index, timeline, thread, begin_ns, end_ns, warmup, budget_ns, tags, facts,
                        zones, runs, spans, samples) ==
               std::tie(other.index, other.timeline, other.thread, other.begin_ns, other.end_ns,
                        other.warmup, other.budget_ns, other.tags, other.facts, other.zones,
                        other.runs, other.spans, other.samples);
    }
} // namespace SushiProfiler
