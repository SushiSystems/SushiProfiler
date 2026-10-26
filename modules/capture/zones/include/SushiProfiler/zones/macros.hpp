/**************************************************************************/
/* macros.hpp                                                             */
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
 * @file macros.hpp
 * @brief Declares the recording macros, each compiled in or out by a level.
 * @author Mustafa Garip
 * @see modules/capture/zones/README.md
 */

#pragma once

#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/sites/site.hpp>
#include <SushiProfiler/zones/records.hpp>
#include <SushiProfiler/zones/zone.hpp>

/// @brief The level the direct macros compile in; a program defines it before including.
#ifndef SP_PROFILE_LEVEL
#define SP_PROFILE_LEVEL 2
#endif

/// @brief Pastes two tokens after expanding them.
#define SP_DETAIL_JOIN(left, right) SP_DETAIL_JOIN_INNER(left, right)

/// @brief Pastes two tokens.
#define SP_DETAIL_JOIN_INNER(left, right) left##right

/// @brief Declares the site and handle of record @p id; the handle is empty when compiled out.
#define SP_DETAIL_SITE(id, compiled_in, name, group, level, role, op, unit)                       \
    static constexpr ::SushiProfiler::Site SP_DETAIL_JOIN(sp_site_, id){                           \
        name, group, __FILE__, __LINE__, level, ::SushiProfiler::SiteRole::role,                   \
        ::SushiProfiler::CounterOp::op, unit};                                                     \
    static ::SushiProfiler::SiteSlot<compiled_in> SP_DETAIL_JOIN(sp_handle_, id)                   \
    {                                                                                              \
        &SP_DETAIL_JOIN(sp_site_, id)                                                              \
    }

/// @brief Declares zone @p id, compiled in when @p level is at most @p compiled.
#define SP_DETAIL_ZONE(id, compiled, level, name, group, name_id)                                 \
    SP_DETAIL_SITE(id, ((level) <= (compiled)), name, group, level, Zone, Add, nullptr);           \
    [[maybe_unused]] const ::SushiProfiler::ScopedZone<((level) <= (compiled))>                    \
        SP_DETAIL_JOIN(sp_zone_, id)(SP_DETAIL_JOIN(sp_handle_, id), name_id)

/// @brief Records the rest of the scope as a zone of @p level, kept when @p compiled reaches it.
#define SP_ZONE_AT(compiled, level, name, group)                                                  \
    SP_DETAIL_ZONE(__COUNTER__, compiled, level, name, group, 0U)

/// @brief Records the rest of the scope as a zone named by the interned @p name_id.
#define SP_ZONE_NAMED_AT(compiled, level, name, group, name_id)                                   \
    SP_DETAIL_ZONE(__COUNTER__, compiled, level, name, group, name_id)

/// @brief Declares counter @p id and records @p value at it.
#define SP_DETAIL_COUNTER(id, compiled, level, name, unit, op, value)                             \
    do                                                                                             \
    {                                                                                              \
        if constexpr ((level) <= (compiled))                                                       \
        {                                                                                          \
            SP_DETAIL_SITE(id, true, name, nullptr, level, Counter, op, unit);                     \
            ::SushiProfiler::record_counter(SP_DETAIL_JOIN(sp_handle_, id), value);                \
        }                                                                                          \
    } while (false)

/// @brief Records @p value at a counter folded per period by CounterOp @p op (Add, Set, Max, Min).
#define SP_COUNTER_AT(compiled, level, name, unit, op, value)                                     \
    SP_DETAIL_COUNTER(__COUNTER__, compiled, level, name, unit, op, value)

/// @brief Declares value site @p id and records @p value at it.
#define SP_DETAIL_VALUE(id, compiled, level, name, unit, value)                                   \
    do                                                                                             \
    {                                                                                              \
        if constexpr ((level) <= (compiled))                                                       \
        {                                                                                          \
            SP_DETAIL_SITE(id, true, name, nullptr, level, Value, Add, unit);                      \
            ::SushiProfiler::record_value(SP_DETAIL_JOIN(sp_handle_, id), value);                  \
        }                                                                                          \
    } while (false)

/// @brief Records @p value as a sample kept as it is.
#define SP_VALUE_AT(compiled, level, name, unit, value)                                           \
    SP_DETAIL_VALUE(__COUNTER__, compiled, level, name, unit, value)

/// @brief Declares instant site @p id and records an occurrence at it.
#define SP_DETAIL_INSTANT(id, compiled, level, name)                                              \
    do                                                                                             \
    {                                                                                              \
        if constexpr ((level) <= (compiled))                                                       \
        {                                                                                          \
            SP_DETAIL_SITE(id, true, name, nullptr, level, Instant, Add, nullptr);                 \
            ::SushiProfiler::record_instant(SP_DETAIL_JOIN(sp_handle_, id));                       \
        }                                                                                          \
    } while (false)

/// @brief Records an occurrence now.
#define SP_INSTANT_AT(compiled, level, name) SP_DETAIL_INSTANT(__COUNTER__, compiled, level, name)

/// @brief Declares flow site @p id and records point @p kind of item @p item at it.
#define SP_DETAIL_FLOW(id, compiled, level, name, kind, item)                                     \
    do                                                                                             \
    {                                                                                              \
        if constexpr ((level) <= (compiled))                                                       \
        {                                                                                          \
            SP_DETAIL_SITE(id, true, name, nullptr, level, Flow, Add, nullptr);                    \
            ::SushiProfiler::record_flow(SP_DETAIL_JOIN(sp_handle_, id),                           \
                                         ::SushiProfiler::EventKind::kind, item);                  \
        }                                                                                          \
    } while (false)

/// @brief Records point @p kind (FlowBegin, FlowStep, FlowEnd) of item @p item's path.
#define SP_FLOW_AT(compiled, level, name, kind, item)                                             \
    SP_DETAIL_FLOW(__COUNTER__, compiled, level, name, kind, item)

/// @brief Puts the rest of the scope in lane @p lane_id, an interned name, when @p compiled is on.
#define SP_LANE_AT(compiled, lane_id)                                                             \
    [[maybe_unused]] const ::SushiProfiler::ScopedLane<((compiled) >= 1)>                          \
        SP_DETAIL_JOIN(sp_lane_, __COUNTER__)(lane_id)

/// @brief Records the rest of the scope as a stage zone.
#define SP_ZONE(name) SP_ZONE_AT(SP_PROFILE_LEVEL, 1, name, nullptr)

/// @brief Records the rest of the scope as a stage zone charged to @p group.
#define SP_ZONE_GROUP(name, group) SP_ZONE_AT(SP_PROFILE_LEVEL, 1, name, group)

/// @brief Records the rest of the scope as a fine zone, level 2.
#define SP_ZONE_FINE(name) SP_ZONE_AT(SP_PROFILE_LEVEL, 2, name, nullptr)

/// @brief Records @p value at a counter folded by CounterOp @p op.
#define SP_COUNTER(name, unit, op, value) SP_COUNTER_AT(SP_PROFILE_LEVEL, 1, name, unit, op, value)

/// @brief Records @p value as a sample.
#define SP_VALUE(name, unit, value) SP_VALUE_AT(SP_PROFILE_LEVEL, 1, name, unit, value)

/// @brief Records an occurrence now.
#define SP_INSTANT(name) SP_INSTANT_AT(SP_PROFILE_LEVEL, 1, name)

/// @brief Records the start of item @p item's path.
#define SP_FLOW_BEGIN(name, item) SP_FLOW_AT(SP_PROFILE_LEVEL, 1, name, FlowBegin, item)

/// @brief Records a point on item @p item's path.
#define SP_FLOW_STEP(name, item) SP_FLOW_AT(SP_PROFILE_LEVEL, 1, name, FlowStep, item)

/// @brief Records the end of item @p item's path.
#define SP_FLOW_END(name, item) SP_FLOW_AT(SP_PROFILE_LEVEL, 1, name, FlowEnd, item)

/// @brief Puts the rest of the scope in lane @p lane_id.
#define SP_LANE(lane_id) SP_LANE_AT(SP_PROFILE_LEVEL, lane_id)

#if SP_PROFILE_LEVEL >= 1
/// @brief Names the calling thread @p name, registering its ring first.
#define SP_THREAD_NAME(name) ::SushiProfiler::Collector::set_thread_name(name)
#else
/// @brief Compiled out: names nothing and does not evaluate @p name.
#define SP_THREAD_NAME(name) static_cast<void>(sizeof(name))
#endif
