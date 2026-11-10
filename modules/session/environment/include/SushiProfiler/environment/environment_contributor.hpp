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
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file environment_contributor.hpp
 * @brief Declares the interface a library implements to add its facts to a record.
 * @author Mustafa Garip
 * @see modules/session/environment/README.md
 */

#pragma once

#include <SushiProfiler/environment/record_facts.hpp>
#include <SushiProfiler/version/export.hpp>

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
    [[nodiscard]] SUSHIPROFILER_API RecordFacts gather_facts(
        const std::vector<const EnvironmentContributor*>& contributors);

    /**
     * @brief Returns @p base with each contributor's facts set in order.
     *
     * The one-argument overload calls it with the host's facts and this library's build; a caller
     * that already holds the base facts, such as a test, reads no machine.
     */
    [[nodiscard]] SUSHIPROFILER_API RecordFacts gather_facts(
        RecordFacts base, const std::vector<const EnvironmentContributor*>& contributors);
} // namespace SushiProfiler
