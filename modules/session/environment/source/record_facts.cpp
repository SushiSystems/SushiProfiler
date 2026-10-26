/**************************************************************************/
/* record_facts.cpp                                                       */
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
 * @file record_facts.cpp
 * @brief Reads the operating system, the CPU brand, the thread count and the compiler.
 * @author Mustafa Garip
 * @see modules/session/environment/README.md
 */

#include <SushiProfiler/environment/record_facts.hpp>

#include <cstring>
#include <thread>
#include <utility>

#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

namespace SushiProfiler
{
    namespace
    {
        /// @brief Returns the operating system family this library was built for.
        std::string os_family()
        {
#if defined(_WIN32)
            return "Windows";
#elif defined(__APPLE__)
            return "macOS";
#elif defined(__linux__)
            return "Linux";
#else
            return "other";
#endif
        }

        /// @brief Reads CPUID leaf @p leaf into @p registers; returns false off x86.
        bool cpuid(unsigned leaf, unsigned (&registers)[4]) noexcept
        {
#if (defined(_M_X64) || defined(_M_IX86)) && defined(_MSC_VER)
            int values[4] = {0, 0, 0, 0};
            __cpuid(values, static_cast<int>(leaf));
            for (int index = 0; index < 4; ++index)
                registers[index] = static_cast<unsigned>(values[index]);
            return true;
#elif defined(__x86_64__) || defined(__i386__)
            return __get_cpuid(leaf, &registers[0], &registers[1], &registers[2],
                               &registers[3]) != 0;
#else
            static_cast<void>(leaf);
            static_cast<void>(registers);
            return false;
#endif
        }

        /// @brief Returns the CPU brand string from CPUID leaves 0x80000002 to 4, or `unknown`.
        std::string cpu_brand()
        {
            unsigned registers[4] = {0U, 0U, 0U, 0U};
            if (!cpuid(0x80000000U, registers) || registers[0] < 0x80000004U)
                return "unknown";
            char brand[49] = {};
            for (unsigned leaf = 0U; leaf < 3U; ++leaf)
            {
                static_cast<void>(cpuid(0x80000002U + leaf, registers));
                std::memcpy(brand + leaf * 16U, registers, 16U);
            }
            std::string text(brand);
            const std::size_t first = text.find_first_not_of(' ');
            const std::size_t last = text.find_last_not_of(' ');
            return first == std::string::npos ? std::string("unknown")
                                              : text.substr(first, last - first + 1U);
        }

        /// @brief Returns the compiler and its version.
        std::string compiler()
        {
#if defined(__clang__)
            return "clang " + std::to_string(__clang_major__) + "." +
                   std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__);
#elif defined(_MSC_VER)
            return "msvc " + std::to_string(_MSC_FULL_VER);
#elif defined(__GNUC__)
            return "gcc " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." +
                   std::to_string(__GNUC_PATCHLEVEL__);
#else
            return "unknown";
#endif
        }
    } // namespace

    void set_fact(std::vector<Fact>& facts, std::string_view name, std::string value)
    {
        for (Fact& fact : facts)
        {
            if (fact.name == name)
            {
                fact.value = std::move(value);
                return;
            }
        }
        facts.push_back(Fact{std::string(name), std::move(value)});
    }

    std::vector<Fact> host_facts()
    {
        return {Fact{"os", os_family()}, Fact{"cpu", cpu_brand()},
                Fact{"hardware_threads", std::to_string(std::thread::hardware_concurrency())}};
    }

    std::vector<Fact> library_build_facts()
    {
#if defined(NDEBUG)
        const char* const build_type = "release";
#else
        const char* const build_type = "debug";
#endif
        return {Fact{"compiler", compiler()}, Fact{"build_type", build_type}};
    }
} // namespace SushiProfiler
