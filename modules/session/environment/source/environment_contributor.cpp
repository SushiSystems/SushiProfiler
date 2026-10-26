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
 * @file environment_contributor.cpp
 * @brief Starts from the host and build facts and lets each contributor set its own.
 * @author Mustafa Garip
 * @see modules/session/environment/README.md
 */

#include <SushiProfiler/environment/environment_contributor.hpp>

namespace SushiProfiler
{
    RecordFacts gather_facts(const std::vector<const EnvironmentContributor*>& contributors)
    {
        RecordFacts facts;
        facts.environment = host_facts();
        facts.build = library_build_facts();
        for (const EnvironmentContributor* const contributor : contributors)
            contributor->contribute(facts);
        return facts;
    }
} // namespace SushiProfiler
