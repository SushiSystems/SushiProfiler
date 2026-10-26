# SushiProfilerTiers.cmake
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

# The module order of the repository, stated once. tools/settings.py holds the checkers' copy.

# Lowest tier first.
set(SUSHIPROFILER_TIER_ORDER
    foundation
    capture
    record
    session
    tools)

# Module, then its tier. A dependency is listed before its consumer.
set(SUSHIPROFILER_MODULE_TIERS
    version     foundation
    clock       foundation
    events      foundation
    json        foundation
    sites       capture
    rings       capture
    collector   capture
    zones       capture
    lock_wait   capture
    periods     capture
    record      record
    record_json record
    statistics  record
    analysis    record
    record_writers record
    compare     record
    checks      record
    config      session
    window      session
    sources     session
    umbrella    tools)

# Consumer, then dependency: the edges allowed between two modules of one tier.
set(SUSHIPROFILER_SAME_TIER_EDGES
    collector      sites
    collector      rings
    zones          sites
    zones          rings
    zones          collector
    lock_wait      sites
    lock_wait      zones
    periods        rings
    periods        collector
    record_json    record
    analysis       record
    analysis       statistics
    record_writers analysis
    record_writers record
    record_writers record_json
    compare        analysis
    compare        record
    compare        statistics
    checks         analysis
    checks         record
    checks         statistics
    window         config)

# Sets <out> to the tier of <module>, or to an empty string for a module the table lacks.
function(sushiprofiler_module_tier module out)
    set(result "")
    list(LENGTH SUSHIPROFILER_MODULE_TIERS length)
    math(EXPR last "${length} - 2")
    foreach(position RANGE 0 ${last} 2)
        list(GET SUSHIPROFILER_MODULE_TIERS ${position} name)
        if(name STREQUAL "${module}")
            math(EXPR tier_position "${position} + 1")
            list(GET SUSHIPROFILER_MODULE_TIERS ${tier_position} result)
            break()
        endif()
    endforeach()
    set(${out} "${result}" PARENT_SCOPE)
endfunction()

# Sets <out> to "<tier>/<module>" for every module, in table order.
function(sushiprofiler_module_folders out)
    set(folders "")
    list(LENGTH SUSHIPROFILER_MODULE_TIERS length)
    math(EXPR last "${length} - 2")
    foreach(position RANGE 0 ${last} 2)
        math(EXPR tier_position "${position} + 1")
        list(GET SUSHIPROFILER_MODULE_TIERS ${position} name)
        list(GET SUSHIPROFILER_MODULE_TIERS ${tier_position} tier)
        list(APPEND folders "${tier}/${name}")
    endforeach()
    set(${out} "${folders}" PARENT_SCOPE)
endfunction()

# Sets <out> to TRUE when SUSHIPROFILER_SAME_TIER_EDGES lists <consumer> -> <dependency>.
function(sushiprofiler_same_tier_edge_allowed consumer dependency out)
    set(result FALSE)
    list(LENGTH SUSHIPROFILER_SAME_TIER_EDGES length)
    if(length GREATER 0)
        math(EXPR last "${length} - 2")
        foreach(position RANGE 0 ${last} 2)
            math(EXPR dependency_position "${position} + 1")
            list(GET SUSHIPROFILER_SAME_TIER_EDGES ${position} edge_consumer)
            list(GET SUSHIPROFILER_SAME_TIER_EDGES ${dependency_position} edge_dependency)
            if(edge_consumer STREQUAL "${consumer}" AND edge_dependency STREQUAL "${dependency}")
                set(result TRUE)
                break()
            endif()
        endforeach()
    endif()
    set(${out} ${result} PARENT_SCOPE)
endfunction()
