/**************************************************************************/
/* span_sink.cpp                                                          */
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
 * @file span_sink.cpp
 * @brief Checks each span's ids and times and each attribute's key and kind before storing it.
 * @author Mustafa Garip
 * @see modules/session/sources/README.md
 */

#include <SushiProfiler/sources/span_sink.hpp>

namespace SushiProfiler
{
    SpanSink::SpanSink(SpanTable& table, const NameTable& names,
                       const std::vector<std::string>& tracks) noexcept
        : table_(table), names_(names), tracks_(tracks)
    {

    }

    std::optional<std::size_t> SpanSink::append(const Span& span)
    {
        const bool ordered = span.queued_ns <= span.start_ns && span.start_ns <= span.end_ns;
        if (span.track >= tracks_.size() || !known_name(span.name) || !ordered)
        {
            ++refused_;
            return std::nullopt;
        }
        return table_.append(span);
    }

    bool SpanSink::set_integer(std::size_t row, NameID key, std::int64_t value)
    {
        if (!accepts(row, key, AttributeKind::Integer))
            return false;
        table_.set_integer(row, key, value);
        return true;
    }

    bool SpanSink::set_real(std::size_t row, NameID key, double value)
    {
        if (!accepts(row, key, AttributeKind::Real))
            return false;
        table_.set_real(row, key, value);
        return true;
    }

    bool SpanSink::set_name(std::size_t row, NameID key, NameID value)
    {
        if (!known_name(value))
        {
            ++refused_;
            return false;
        }
        if (!accepts(row, key, AttributeKind::Name))
            return false;
        table_.set_name(row, key, value);
        return true;
    }

    std::uint64_t SpanSink::refused() const noexcept
    {
        return refused_;
    }

    bool SpanSink::known_name(NameID id) const
    {
        return id <= names_.size();
    }

    bool SpanSink::accepts(std::size_t row, NameID key, AttributeKind kind)
    {
        const AttributeColumn* const column = table_.attribute(key);
        const bool fits = column == nullptr || column->kind == kind;
        if (row >= table_.size() || key == 0U || !known_name(key) || !fits)
        {
            ++refused_;
            return false;
        }
        return true;
    }
} // namespace SushiProfiler
