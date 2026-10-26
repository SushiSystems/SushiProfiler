# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

sushiprofiler_add_module(
    NAME analysis TIER record
    SOURCES
        analysis_catalog.cpp
        analysis_runner.cpp
        breakdown_analysis.cpp
        budget_analysis.cpp
        builtin_analyses.cpp
        counters_analysis.cpp
        device_idle_analysis.cpp
        distribution_analysis.cpp
        flows_analysis.cpp
        overhead_analysis.cpp
        record_queries.cpp
        sample_summary.cpp
        subject_samples.cpp
        short_spans_analysis.cpp
        table.cpp
        top_spans_analysis.cpp
    DEPENDS record statistics
    PRIVATE_DEPENDS events sites)
