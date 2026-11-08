/**************************************************************************/
/* memory_contributor.cpp                                                 */
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
 * @file memory_contributor.cpp
 * @brief States the facts the in-memory environment contributor holds.
 * @author Mustafa Garip
 * @see modules/session/environment/README.md
 */

#include <SushiProfiler/environment/memory_contributor.hpp>

#include <utility>

namespace SushiProfiler
{
    void MemoryContributor::state_environment(const std::string& name, std::string value)
    {
        set_fact(held_.environment, name, std::move(value));
    }

    void MemoryContributor::state_build(const std::string& name, std::string value)
    {
        set_fact(held_.build, name, std::move(value));
    }

    void MemoryContributor::contribute(RecordFacts& facts) const
    {
        for (const Fact& fact : held_.environment)
            set_fact(facts.environment, fact.name, fact.value);
        for (const Fact& fact : held_.build)
            set_fact(facts.build, fact.name, fact.value);
    }
} // namespace SushiProfiler
