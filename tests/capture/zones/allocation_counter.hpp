/**************************************************************************/
/* allocation_counter.hpp                                                 */
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
 * @file allocation_counter.hpp
 * @brief Declares the count of global allocations each thread of the test program makes.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#pragma once

#include <cstdint>

namespace SushiProfilerTests
{
    /// @brief Returns how many times the calling thread called the global operator new.
    [[nodiscard]] std::uint64_t allocations_on_this_thread() noexcept;
} // namespace SushiProfilerTests
