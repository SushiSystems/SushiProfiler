/**************************************************************************/
/* record_names.cpp                                                       */
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
 * @file record_names.cpp
 * @brief Maps roles, operations, attribute kinds and flow points to the format's words and back.
 * @author Mustafa Garip
 * @see modules/record/record_json/README.md
 */

#include "record_names.hpp"

#include <array>
#include <cstddef>
#include <utility>

namespace SushiProfiler::Detail
{
    namespace
    {
        /// @brief The words of SiteRole, in its order.
        constexpr std::array<std::string_view, 6> K_ROLES = {"zone",    "lock_wait", "instant",
                                                             "counter", "value",     "flow"};

        /// @brief The words of CounterOp, in its order.
        constexpr std::array<std::string_view, 4> K_OPS = {"add", "set", "max", "min"};

        /// @brief The words of AttributeKind, in its order.
        constexpr std::array<std::string_view, 3> K_KINDS = {"integer", "real", "name"};

        /// @brief The words of FlowPoint, in its order.
        constexpr std::array<std::string_view, 3> K_POINTS = {"begin", "step", "end"};

        /// @brief Returns the enumerator whose word in @p words is @p word, or none.
        template <class Enum, std::size_t Count>
        std::optional<Enum> lookup(const std::array<std::string_view, Count>& words,
                                   std::string_view word) noexcept
        {
            for (std::size_t index = 0U; index < Count; ++index)
            {
                if (words[index] == word)
                    return static_cast<Enum>(index);
            }
            return std::nullopt;
        }
    } // namespace

    std::string_view role_word(SiteRole role) noexcept
    {
        return K_ROLES[static_cast<std::size_t>(role)];
    }

    std::optional<SiteRole> role_from(std::string_view word) noexcept
    {
        return lookup<SiteRole>(K_ROLES, word);
    }

    std::string_view op_word(CounterOp op) noexcept
    {
        return K_OPS[static_cast<std::size_t>(op)];
    }

    std::optional<CounterOp> op_from(std::string_view word) noexcept
    {
        return lookup<CounterOp>(K_OPS, word);
    }

    std::string_view kind_word(AttributeKind kind) noexcept
    {
        return K_KINDS[static_cast<std::size_t>(kind)];
    }

    std::optional<AttributeKind> kind_from(std::string_view word) noexcept
    {
        return lookup<AttributeKind>(K_KINDS, word);
    }

    std::string_view point_word(FlowPoint point) noexcept
    {
        return K_POINTS[static_cast<std::size_t>(point)];
    }

    std::optional<FlowPoint> point_from(std::string_view word) noexcept
    {
        return lookup<FlowPoint>(K_POINTS, word);
    }
} // namespace SushiProfiler::Detail
