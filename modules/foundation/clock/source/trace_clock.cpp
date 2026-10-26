/**************************************************************************/
/* trace_clock.cpp                                                        */
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
 * @file trace_clock.cpp
 * @brief Asks the processor once whether its time-stamp counter is invariant.
 * @author Mustafa Garip
 * @see modules/foundation/clock/README.md
 */

#include <SushiProfiler/clock/trace_clock.hpp>

#if defined(SP_CLOCK_HAS_TSC) && !defined(_MSC_VER)
#include <cpuid.h>
#endif

namespace SushiProfiler
{
    namespace
    {
        /// @brief Reads CPUID leaf 0x80000007, whose EDX bit 8 is the invariant-TSC flag.
        bool query_invariant_tsc() noexcept
        {
#if defined(SP_CLOCK_HAS_TSC) && defined(_MSC_VER)
            int registers[4] = {0, 0, 0, 0};
            __cpuid(registers, static_cast<int>(0x80000000U));
            if (static_cast<unsigned>(registers[0]) < 0x80000007U)
                return false;
            __cpuid(registers, static_cast<int>(0x80000007U));
            return (static_cast<unsigned>(registers[3]) & (1U << 8U)) != 0U;
#elif defined(SP_CLOCK_HAS_TSC)
            unsigned eax = 0U;
            unsigned ebx = 0U;
            unsigned ecx = 0U;
            unsigned edx = 0U;
            if (__get_cpuid(0x80000007U, &eax, &ebx, &ecx, &edx) == 0)
                return false;
            return (edx & (1U << 8U)) != 0U;
#else
            return false;
#endif
        }
    } // namespace

    bool TraceClock::ticks_are_invariant() noexcept
    {
        static const bool invariant = query_invariant_tsc();
        return invariant;
    }
} // namespace SushiProfiler
