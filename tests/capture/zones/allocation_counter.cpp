/**************************************************************************/
/* allocation_counter.cpp                                                 */
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
 * @file allocation_counter.cpp
 * @brief Replaces the global operator new and delete with malloc and free that count per thread.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#include "allocation_counter.hpp"

#include <cstdlib>
#include <new>

namespace
{
    /// @brief The calling thread's calls to the global operator new.
    thread_local std::uint64_t t_allocations = 0U;

    /// @brief Counts one allocation and returns @p size bytes from malloc, or null when it fails.
    void* counted_allocate_or_null(std::size_t size) noexcept
    {
        ++t_allocations;
        return std::malloc(size != 0U ? size : 1U);
    }

    /// @brief Counts one allocation and returns @p size bytes from malloc.
    void* counted_allocate(std::size_t size)
    {
        void* const memory = counted_allocate_or_null(size);
        if (memory == nullptr)
            throw std::bad_alloc();
        return memory;
    }
} // namespace

namespace SushiProfilerTests
{
    std::uint64_t allocations_on_this_thread() noexcept
    {
        return t_allocations;
    }
} // namespace SushiProfilerTests

/// @brief Counts and allocates one object.
void* operator new(std::size_t size)
{
    return counted_allocate(size);
}

/// @brief Counts and allocates one array.
void* operator new[](std::size_t size)
{
    return counted_allocate(size);
}

/// @brief Counts and allocates one object, returning null when it fails. A sanitizer runtime
/// provides this form itself, so it is replaced too, or its memory would meet our free.
void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    return counted_allocate_or_null(size);
}

/// @brief Counts and allocates one array, returning null when it fails.
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
    return counted_allocate_or_null(size);
}

/// @brief Frees one object.
void operator delete(void* memory) noexcept
{
    std::free(memory);
}

/// @brief Frees one array.
void operator delete[](void* memory) noexcept
{
    std::free(memory);
}

/// @brief Frees one object of a known size.
void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}

/// @brief Frees one array of a known size.
void operator delete[](void* memory, std::size_t) noexcept
{
    std::free(memory);
}

/// @brief Frees one object whose nothrow allocation was followed by a throwing constructor.
void operator delete(void* memory, const std::nothrow_t&) noexcept
{
    std::free(memory);
}

/// @brief Frees one array whose nothrow allocation was followed by a throwing constructor.
void operator delete[](void* memory, const std::nothrow_t&) noexcept
{
    std::free(memory);
}
