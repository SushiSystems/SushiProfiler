/**************************************************************************/
/* memory_contributor.hpp                                                 */
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
 * @file memory_contributor.hpp
 * @brief Declares the in-memory environment contributor, which states the facts it was given.
 * @author Mustafa Garip
 * @see modules/session/environment/README.md
 */

#pragma once

#include <SushiProfiler/environment/environment_contributor.hpp>
#include <SushiProfiler/environment/record_facts.hpp>
#include <SushiProfiler/version/export.hpp>

#include <string>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief States the environment and build facts it holds, in the order they were given.
    class SUSHIPROFILER_API MemoryContributor final : public EnvironmentContributor
    {
        public:
            /// @brief Holds environment fact @p name = @p value, replacing a value held before.
            void state_environment(const std::string& name, std::string value);

            /// @brief Holds build fact @p name = @p value, replacing a value held before.
            void state_build(const std::string& name, std::string value);

            /// @brief Sets every held fact in @p facts with set_fact(), environment first.
            void contribute(RecordFacts& facts) const override;

        private:
            /// @brief The facts held.
            RecordFacts held_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
