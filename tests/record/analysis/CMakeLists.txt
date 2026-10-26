# CMakeLists.txt
# SushiProfiler - https://github.com/sushisystems/sushiprofiler
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.

sushiprofiler_add_module_tests(
    MODULE analysis
    SOURCES
        analysis_record.cpp
        test_analysis.cpp
        test_analysis_catalog.cpp
        test_analysis_runner.cpp
        test_breakdown_analysis.cpp
        test_budget_analysis.cpp
        test_counters_analysis.cpp
        test_device_idle_analysis.cpp
        test_distribution_analysis.cpp
        test_flows_analysis.cpp
        test_overhead_analysis.cpp
        test_roofline_analysis.cpp
        test_sample_summary.cpp
        test_short_spans_analysis.cpp
        test_subject_samples.cpp
        test_table.cpp
        test_top_spans_analysis.cpp)
