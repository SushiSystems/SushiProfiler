/**************************************************************************/
/* benchmark_consumer.cpp                                                 */
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
 * @file benchmark_consumer.cpp
 * @brief Measures how often the consumer wakes while the collector records nothing.
 * @author Mustafa Garip
 * @see tests/session/consumer/README.md
 */

#include <SushiProfiler/consumer/consumer.hpp>

#include "assembly_rig.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/record/run_record.hpp>

namespace
{
    /// @brief Idle time before the measurement, long enough for the wait to reach its slowest.
    constexpr std::chrono::seconds K_SETTLE{2};

    /// @brief Idle time measured.
    constexpr std::chrono::seconds K_MEASURED{8};
} // namespace

/// @brief Prints the consumer's drains per second over an idle stretch.
int main()
{
    using SushiProfiler::Collector;
    Collector& collector = Collector::instance();
    if (!collector.open(SushiProfiler::CollectorSettings{}))
    {
        std::fprintf(stderr, "sp_bench_consumer: the collector could not open\n");
        return 1;
    }
    SushiProfiler::RunRecord record;
    SushiProfiler::AssemblySettings settings;
    settings.origin = collector.first_point();
    settings.run_timeline = collector.names().intern("run");
    SushiProfilerTests::AssemblyRig rig;
    SushiProfiler::PeriodAssembler assembler(settings, record, collector.names(),
                                             collector.sites(), {}, rig.parts());
    SushiProfiler::Consumer consumer(collector, assembler);

    consumer.start();
    std::this_thread::sleep_for(K_SETTLE);
    const std::uint64_t before = consumer.drains();
    std::this_thread::sleep_for(K_MEASURED);
    const std::uint64_t after = consumer.drains();
    collector.close();
    consumer.stop();

    const double seconds = std::chrono::duration<double>(K_MEASURED).count();
    std::printf("drainer_idle_wakeups %.3f per_s\n", static_cast<double>(after - before) / seconds);
    return 0;
}
