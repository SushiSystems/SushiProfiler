/**************************************************************************/
/* named_macros.hpp                                                       */
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
 * @file named_macros.hpp
 * @brief Declares the counter and value macros that take a name built at run time.
 * @author Mustafa Garip
 * @see modules/capture/zones/README.md
 */

#pragma once

#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/zones/macros.hpp>
#include <SushiProfiler/zones/named_records.hpp>

/**
 * @brief Records @p value at a counter named by @p name_expression, kept when @p compiled
 *        reaches @p level.
 *
 * The expression is evaluated only while the collector records, so building a name costs nothing
 * with the gate closed. @p op is Add, Set, Max or Min.
 */
#define SP_COUNTER_NAMED_AT(compiled, level, name_expression, unit, op, value)                    \
    do                                                                                             \
    {                                                                                              \
        if constexpr ((level) <= (compiled))                                                       \
        {                                                                                          \
            if (::SushiProfiler::Collector::recording())                                           \
            {                                                                                      \
                ::SushiProfiler::record_counter_named((name_expression), (unit),                   \
                                                      ::SushiProfiler::CounterOp::op, (level),     \
                                                      (value));                                    \
            }                                                                                      \
        }                                                                                          \
    } while (false)

/// @brief Records @p value as a sample of the value named by @p name_expression.
#define SP_VALUE_NAMED_AT(compiled, level, name_expression, unit, value)                          \
    do                                                                                             \
    {                                                                                              \
        if constexpr ((level) <= (compiled))                                                       \
        {                                                                                          \
            if (::SushiProfiler::Collector::recording())                                           \
            {                                                                                      \
                ::SushiProfiler::record_value_named((name_expression), (unit), (level), (value)); \
            }                                                                                      \
        }                                                                                          \
    } while (false)

/// @brief Records @p value at the counter named by @p name_expression, level 1.
#define SP_COUNTER_NAMED(name_expression, unit, op, value)                                        \
    SP_COUNTER_NAMED_AT(SP_PROFILE_LEVEL, 1, name_expression, unit, op, value)

/// @brief Records @p value as a sample of the value named by @p name_expression, level 1.
#define SP_VALUE_NAMED(name_expression, unit, value)                                              \
    SP_VALUE_NAMED_AT(SP_PROFILE_LEVEL, 1, name_expression, unit, value)
