/**************************************************************************/
/* table.cpp                                                              */
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
 * @file table.cpp
 * @brief Builds cells and columns and finds a column by name.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#include <SushiProfiler/analysis/table.hpp>

#include <utility>

namespace SushiProfiler
{
    bool Column::operator==(const Column& other) const
    {
        return name == other.name && unit == other.unit;
    }

    Cell Cell::text(std::string value)
    {
        Cell cell;
        cell.content_ = std::move(value);
        return cell;
    }

    Cell Cell::whole(std::int64_t value)
    {
        Cell cell;
        cell.content_ = value;
        return cell;
    }

    Cell Cell::real(double value)
    {
        Cell cell;
        cell.content_ = value;
        return cell;
    }

    bool Cell::empty() const noexcept
    {
        return std::holds_alternative<std::monostate>(content_);
    }

    const std::string* Cell::as_text() const noexcept
    {
        return std::get_if<std::string>(&content_);
    }

    const std::int64_t* Cell::as_whole() const noexcept
    {
        return std::get_if<std::int64_t>(&content_);
    }

    const double* Cell::as_real() const noexcept
    {
        return std::get_if<double>(&content_);
    }

    bool Cell::operator==(const Cell& other) const
    {
        return content_ == other.content_;
    }

    bool Cell::operator!=(const Cell& other) const
    {
        return !(*this == other);
    }

    void Table::add_column(std::string column_name, Unit unit)
    {
        columns.push_back(Column{std::move(column_name), unit});
    }

    std::size_t Table::column(const std::string& column_name) const noexcept
    {
        for (std::size_t index = 0U; index < columns.size(); ++index)
        {
            if (columns[index].name == column_name)
                return index;
        }
        return columns.size();
    }
} // namespace SushiProfiler
