/**************************************************************************/
/* record_facts.hpp                                                       */
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
 * @file record_facts.hpp
 * @brief Declares the environment and build facts of a record, and how a fact is set.
 * @author Mustafa Garip
 * @see modules/session/environment/README.md
 */

#pragma once

#include <SushiProfiler/record/period_record.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace SushiProfiler
{
    /// @brief The facts a record states about where and how it was made.
    struct RecordFacts
    {
        std::vector<Fact> environment; /**< The host and the libraries, in the order set. */
        std::vector<Fact> build;       /**< What a comparison requires to match. */
    };

    /// @brief Sets fact @p name of @p facts to @p value, replacing its value when it is there.
    void set_fact(std::vector<Fact>& facts, std::string_view name, std::string value);

    /// @brief Returns the host's facts: `os`, `cpu` and `hardware_threads`.
    [[nodiscard]] std::vector<Fact> host_facts();

    /// @brief Returns how this library was built: `compiler` and `build_type`.
    [[nodiscard]] std::vector<Fact> library_build_facts();
} // namespace SushiProfiler
