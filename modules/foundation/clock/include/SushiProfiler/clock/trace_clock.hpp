/**************************************************************************/
/* trace_clock.hpp                                                        */
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
 * @file trace_clock.hpp
 * @brief Declares the clock every event is stamped with: the TSC where it is invariant.
 * @author Mustafa Garip
 * @see modules/foundation/clock/README.md
 */

#pragma once

#include <SushiProfiler/clock/calibration.hpp>
#include <SushiProfiler/version/export.hpp>

#include <chrono>
#include <cstdint>

#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
#define SP_CLOCK_HAS_TSC 1
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /**
     * @brief Reads the processor's time-stamp counter and the steady clock.
     *
     * `now()` is one instruction and no system call. Its ticks map onto steady time only when
     * `ticks_are_invariant()` holds; a caller that finds it false stamps with `steady_now_ns()`.
     */
    class SUSHIPROFILER_API TraceClock
    {
        public:
            TraceClock() = delete;

            /// @brief Returns the time-stamp counter, or steady nanoseconds without one.
            [[nodiscard]] static std::uint64_t now() noexcept
            {
#if defined(SP_CLOCK_HAS_TSC)
                return static_cast<std::uint64_t>(__rdtsc());
#else
                return steady_now_ns();
#endif
            }

            /// @brief Returns the steady clock in nanoseconds since its epoch.
            [[nodiscard]] static std::uint64_t steady_now_ns() noexcept
            {
                const auto since_epoch = std::chrono::steady_clock::now().time_since_epoch();
                return static_cast<std::uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch).count());
            }

            /// @brief Returns whether the counter runs at one rate in every power state and core.
            [[nodiscard]] static bool ticks_are_invariant() noexcept;

            /**
             * @brief Pairs one tick reading with one steady reading.
             *
             * Of 16 brackets of two tick reads around a steady read, keeps the tightest, so the
             * pairing errs by at most half its width.
             */
            [[nodiscard]] static CalibrationPoint sample() noexcept
            {
                CalibrationPoint point;
                std::uint64_t tightest = ~std::uint64_t{0};
                for (int attempt = 0; attempt < K_SAMPLE_ATTEMPTS; ++attempt)
                {
                    const std::uint64_t before = now();
                    const std::uint64_t steady = steady_now_ns();
                    const std::uint64_t after = now();
                    if (after - before < tightest)
                    {
                        tightest = after - before;
                        point.ticks = before + (after - before) / 2U;
                        point.steady_ns = steady;
                    }
                }
                return point;
            }

        private:
            /// @brief Brackets `sample()` takes before it keeps the tightest.
            static constexpr int K_SAMPLE_ATTEMPTS = 16;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
