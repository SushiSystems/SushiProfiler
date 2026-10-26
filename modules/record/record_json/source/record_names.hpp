/**************************************************************************/
/* record_names.hpp                                                       */
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
 * @file record_names.hpp
 * @brief Declares the words the record format writes for roles, operations, kinds and points.
 * @author Mustafa Garip
 * @see modules/record/record_json/README.md
 */

#pragma once

#include <SushiProfiler/record/sample_series.hpp>
#include <SushiProfiler/record/span_table.hpp>
#include <SushiProfiler/sites/site.hpp>

#include <optional>
#include <string_view>

namespace SushiProfiler::Detail
{
    /// @brief Returns the format's word for @p role.
    [[nodiscard]] std::string_view role_word(SiteRole role) noexcept;

    /// @brief Returns the role written as @p word, or none.
    [[nodiscard]] std::optional<SiteRole> role_from(std::string_view word) noexcept;

    /// @brief Returns the format's word for @p op.
    [[nodiscard]] std::string_view op_word(CounterOp op) noexcept;

    /// @brief Returns the operation written as @p word, or none.
    [[nodiscard]] std::optional<CounterOp> op_from(std::string_view word) noexcept;

    /// @brief Returns the format's word for @p kind.
    [[nodiscard]] std::string_view kind_word(AttributeKind kind) noexcept;

    /// @brief Returns the attribute kind written as @p word, or none.
    [[nodiscard]] std::optional<AttributeKind> kind_from(std::string_view word) noexcept;

    /// @brief Returns the format's word for @p point.
    [[nodiscard]] std::string_view point_word(FlowPoint point) noexcept;

    /// @brief Returns the flow point written as @p word, or none.
    [[nodiscard]] std::optional<FlowPoint> point_from(std::string_view word) noexcept;
} // namespace SushiProfiler::Detail
