/**************************************************************************/
/* environment_contributor.hpp                                            */
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
 * @file environment_contributor.hpp
 * @brief Declares the interface a library implements to add its facts to a record.
 * @author Mustafa Garip
 * @see modules/session/environment/README.md
 */

#pragma once

#include <SushiProfiler/environment/record_facts.hpp>

#include <vector>

namespace SushiProfiler
{
    /// @brief Adds a library's facts, such as its name, commit or device, as a session starts.
    class EnvironmentContributor
    {
        public:
            virtual ~EnvironmentContributor() = default;

            /// @brief Sets this contributor's facts in @p facts with set_fact().
            virtual void contribute(RecordFacts& facts) const = 0;
    };

    /**
     * @brief Returns the host's facts and this library's build, then each contributor's facts.
     *
     * Contributors run in order; a fact set again replaces the earlier value in place, so a host
     * can state its own `build_type` over the library's.
     */
    [[nodiscard]] RecordFacts gather_facts(
        const std::vector<const EnvironmentContributor*>& contributors);
} // namespace SushiProfiler
