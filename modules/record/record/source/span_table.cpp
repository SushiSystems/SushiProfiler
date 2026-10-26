/**************************************************************************/
/* span_table.cpp                                                         */
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
 * @file span_table.cpp
 * @brief Appends spans and attributes while keeping every column one entry per span.
 * @author Mustafa Garip
 * @see modules/record/record/README.md
 */

#include <SushiProfiler/record/span_table.hpp>

#include <tuple>
#include <utility>

namespace SushiProfiler
{
    bool AttributeColumn::operator==(const AttributeColumn& other) const
    {
        return std::tie(key, kind, integers, reals, present) ==
               std::tie(other.key, other.kind, other.integers, other.reals, other.present);
    }

    std::size_t SpanTable::append(const Span& span)
    {
        queued_ns_.push_back(span.queued_ns);
        start_ns_.push_back(span.start_ns);
        end_ns_.push_back(span.end_ns);
        track_.push_back(span.track);
        name_.push_back(span.name);
        submit_thread_.push_back(span.submit_thread);
        uncertainty_ns_.push_back(span.uncertainty_ns);
        for (AttributeColumn& column : attributes_)
        {
            column.integers.push_back(0);
            column.reals.push_back(0.0);
            column.present.push_back(0U);
        }
        return queued_ns_.size() - 1U;
    }

    AttributeColumn& SpanTable::column(NameID key, AttributeKind kind)
    {
        for (AttributeColumn& existing : attributes_)
        {
            if (existing.key == key)
                return existing;
        }
        AttributeColumn added;
        added.key = key;
        added.kind = kind;
        added.integers.assign(size(), 0);
        added.reals.assign(size(), 0.0);
        added.present.assign(size(), 0U);
        attributes_.push_back(std::move(added));
        return attributes_.back();
    }

    void SpanTable::set_integer(std::size_t row, NameID key, std::int64_t value)
    {
        AttributeColumn& target = column(key, AttributeKind::Integer);
        target.integers[row] = value;
        target.present[row] = 1U;
    }

    void SpanTable::set_real(std::size_t row, NameID key, double value)
    {
        AttributeColumn& target = column(key, AttributeKind::Real);
        target.reals[row] = value;
        target.present[row] = 1U;
    }

    void SpanTable::set_name(std::size_t row, NameID key, NameID value)
    {
        AttributeColumn& target = column(key, AttributeKind::Name);
        target.integers[row] = static_cast<std::int64_t>(value);
        target.present[row] = 1U;
    }

    std::size_t SpanTable::size() const noexcept
    {
        return queued_ns_.size();
    }

    Span SpanTable::span(std::size_t row) const
    {
        Span span;
        span.queued_ns = queued_ns_[row];
        span.start_ns = start_ns_[row];
        span.end_ns = end_ns_[row];
        span.track = track_[row];
        span.name = name_[row];
        span.submit_thread = submit_thread_[row];
        span.uncertainty_ns = uncertainty_ns_[row];
        return span;
    }

    const AttributeColumn* SpanTable::attribute(NameID key) const noexcept
    {
        for (const AttributeColumn& column : attributes_)
        {
            if (column.key == key)
                return &column;
        }
        return nullptr;
    }

    const std::vector<AttributeColumn>& SpanTable::attributes() const noexcept
    {
        return attributes_;
    }

    void SpanTable::add_column(AttributeColumn column)
    {
        attributes_.push_back(std::move(column));
    }

    bool SpanTable::operator==(const SpanTable& other) const
    {
        return std::tie(queued_ns_, start_ns_, end_ns_, track_, name_, submit_thread_,
                        uncertainty_ns_, attributes_) ==
               std::tie(other.queued_ns_, other.start_ns_, other.end_ns_, other.track_,
                        other.name_, other.submit_thread_, other.uncertainty_ns_,
                        other.attributes_);
    }
} // namespace SushiProfiler
