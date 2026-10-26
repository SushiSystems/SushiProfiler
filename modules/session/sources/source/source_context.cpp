/**************************************************************************/
/* source_context.cpp                                                     */
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
 * @file source_context.cpp
 * @brief Interns names, adds tracks once each and moves steady times onto the record's clock.
 * @author Mustafa Garip
 * @see modules/session/sources/README.md
 */

#include <SushiProfiler/sources/source_context.hpp>

namespace SushiProfiler
{
    SourceContext::SourceContext(NameTable& names, std::vector<std::string>& tracks,
                                 std::uint64_t origin_steady_ns) noexcept
        : names_(names), tracks_(tracks), origin_steady_ns_(origin_steady_ns)
    {

    }

    NameID SourceContext::intern(std::string_view text)
    {
        return names_.intern(text);
    }

    TrackID SourceContext::add_track(std::string_view name)
    {
        for (std::size_t index = 0U; index < tracks_.size(); ++index)
        {
            if (tracks_[index] == name)
                return static_cast<TrackID>(index);
        }
        tracks_.emplace_back(name);
        return static_cast<TrackID>(tracks_.size() - 1U);
    }

    std::uint64_t SourceContext::to_record_ns(std::uint64_t steady_ns) const noexcept
    {
        return steady_ns > origin_steady_ns_ ? steady_ns - origin_steady_ns_ : 0U;
    }
} // namespace SushiProfiler
