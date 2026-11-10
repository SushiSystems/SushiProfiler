/**************************************************************************/
/* environment_contributor.cpp                                            */
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
 * @file environment_contributor.cpp
 * @brief Starts from the host and build facts and lets each contributor set its own.
 * @author Mustafa Garip
 * @see modules/session/environment/README.md
 */

#include <SushiProfiler/environment/environment_contributor.hpp>

#include <utility>

namespace SushiProfiler
{
    RecordFacts gather_facts(const std::vector<const EnvironmentContributor*>& contributors)
    {
        RecordFacts base;
        base.environment = host_facts();
        base.build = library_build_facts();
        return gather_facts(std::move(base), contributors);
    }

    RecordFacts gather_facts(RecordFacts base,
                             const std::vector<const EnvironmentContributor*>& contributors)
    {
        for (const EnvironmentContributor* const contributor : contributors)
            contributor->contribute(base);
        return base;
    }
} // namespace SushiProfiler
