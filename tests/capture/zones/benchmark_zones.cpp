/**************************************************************************/
/* benchmark_zones.cpp                                                    */
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
 * @file benchmark_zones.cpp
 * @brief Measures a zone compiled out, gated off, disabled and recording, counters and the clocks.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#include <SushiProfiler/clock/trace_clock.hpp>
#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/zones/macros.hpp>
#include <SushiProfiler/zones/named_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace
{
    /// @brief Runs measured and discarded before the measured ones.
    constexpr int K_WARM_UP_RUNS = 3;

    /// @brief Runs measured; the median is reported.
    constexpr int K_RUNS = 31;

    /// @brief Iterations of one run.
    constexpr std::uint64_t K_ITERATIONS = 1'000'000U;

    /// @brief The ring of the measuring thread, large enough for one run of single events.
    constexpr std::size_t K_RING_EVENTS = std::size_t{1U} << 21U;

    /// @brief Where the clock cases store each reading, so the compiler keeps the read.
    volatile std::uint64_t g_sink = 0U;

    /// @brief Empties every ring outside the measured time.
    void drain_all()
    {
        SushiProfiler::DrainBatch batch;
        SushiProfiler::Collector::instance().drain(batch);
    }

    /// @brief Returns the median nanoseconds per iteration of @p body over K_RUNS runs.
    template <class Body>
    double median_nanoseconds(Body body)
    {
        std::vector<double> runs;
        runs.reserve(K_RUNS);
        for (int run = 0; run < K_WARM_UP_RUNS + K_RUNS; ++run)
        {
            drain_all();
            const auto start = std::chrono::steady_clock::now();
            for (std::uint64_t iteration = 0U; iteration < K_ITERATIONS; ++iteration)
                body(iteration);
            const auto end = std::chrono::steady_clock::now();
            if (run >= K_WARM_UP_RUNS)
            {
                const double total = std::chrono::duration<double, std::nano>(end - start).count();
                runs.push_back(total / static_cast<double>(K_ITERATIONS));
            }
        }
        const auto middle = runs.begin() + static_cast<std::ptrdiff_t>(runs.size() / 2U);
        std::nth_element(runs.begin(), middle, runs.end());
        return *middle;
    }

    /// @brief Prints one measurement as `<name> <value> ns`.
    void report(const char* name, double nanoseconds)
    {
        std::printf("%s %.3f ns\n", name, nanoseconds);
    }
} // namespace

/// @brief Measures each case and prints one line per case.
int main()
{
    using SushiProfiler::Collector;
    if (!Collector::register_this_thread(K_RING_EVENTS) ||
        !Collector::instance().open(SushiProfiler::CollectorSettings{}))
    {
        std::fprintf(stderr, "sp_bench_zones: the collector could not open\n");
        return 1;
    }

    report("zone_compiled_out", median_nanoseconds([](std::uint64_t) {
        SP_ZONE_AT(0, 1, "compiled_out", nullptr);
    }));

    Collector::instance().close();
    report("zone_gate_closed", median_nanoseconds([](std::uint64_t) {
        SP_ZONE_AT(3, 1, "gate_closed", nullptr);
    }));
    drain_all();
    Collector::instance().open(SushiProfiler::CollectorSettings{});

    SushiProfiler::SiteFilter coarse;
    coarse.level = 1U;
    Collector::instance().apply(coarse);
    report("zone_site_disabled", median_nanoseconds([](std::uint64_t) {
        SP_ZONE_AT(3, 2, "site_disabled", nullptr);
    }));
    Collector::instance().apply(SushiProfiler::SiteFilter{});

    report("zone_recording", median_nanoseconds([](std::uint64_t) {
        SP_ZONE_AT(3, 1, "recording", nullptr);
    }));
    report("counter_recording", median_nanoseconds([](std::uint64_t iteration) {
        SP_COUNTER_AT(3, 1, "counter", "", Add, static_cast<double>(iteration));
    }));
    // Outside the 25 ns recording row: a shared lock and a map lookup per record.
    report("counter_named_recording", median_nanoseconds([](std::uint64_t iteration) {
        SP_COUNTER_NAMED_AT(3, 1, "named_counter", "", Add, static_cast<double>(iteration));
    }));
    report("clock_now", median_nanoseconds([](std::uint64_t) {
        g_sink = SushiProfiler::TraceClock::now();
    }));
    report("steady_now", median_nanoseconds([](std::uint64_t) {
        g_sink = SushiProfiler::TraceClock::steady_now_ns();
    }));

    Collector::instance().close();
    drain_all();
    return 0;
}
