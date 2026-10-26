/**************************************************************************/
/* export.hpp                                                             */
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
 * @file export.hpp
 * @brief Defines SUSHIPROFILER_API, which exports a symbol from the shared library.
 * @author Mustafa Garip
 */

#pragma once

/**
 * @brief Marks a class or function the library defines and a consumer calls.
 *
 * The static target defines SUSHIPROFILER_STATIC, which leaves every symbol hidden in the binary
 * that links it. The shared library is built with SUSHIPROFILER_EXPORTS.
 */
#if defined(SUSHIPROFILER_STATIC)
    #define SUSHIPROFILER_API
#elif defined(_WIN32) || defined(__CYGWIN__)
    #if defined(SUSHIPROFILER_EXPORTS)
        #define SUSHIPROFILER_API __declspec(dllexport)
    #else
        #define SUSHIPROFILER_API __declspec(dllimport)
    #endif
#else
    #define SUSHIPROFILER_API __attribute__((visibility("default")))
#endif

/**
 * @brief Opens a region in which MSVC does not warn about standard library members of an exported
 * class (C4251) or a base class that is not exported (C4275).
 */
#if defined(_MSC_VER)
    #define SUSHIPROFILER_DLL_WARNINGS_PUSH \
        __pragma(warning(push)) __pragma(warning(disable : 4251 4275))
#else
    #define SUSHIPROFILER_DLL_WARNINGS_PUSH
#endif

/// @brief Closes the region SUSHIPROFILER_DLL_WARNINGS_PUSH opened.
#if defined(_MSC_VER)
    #define SUSHIPROFILER_DLL_WARNINGS_POP __pragma(warning(pop))
#else
    #define SUSHIPROFILER_DLL_WARNINGS_POP
#endif
