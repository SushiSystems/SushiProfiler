/**************************************************************************/
/* compile_site.cpp                                                          */
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
 * @file compile_site.cpp
 * @brief Checks at compile time that a site is a constant aggregate built where it is declared.
 * @author Mustafa Garip
 * @see tests/capture/sites/README.md
 */

#include <SushiProfiler/sites/site.hpp>

#include <type_traits>

namespace
{
    using SushiProfiler::CounterOp;
    using SushiProfiler::Site;
    using SushiProfiler::SiteRole;

    /// @brief A site built at compile time, as the recording macros build theirs.
    constexpr Site K_SITE{"frame", nullptr, "compile_site.cpp", 7U, 1U, SiteRole::Zone,
                          CounterOp::Add, nullptr};
    static_assert(std::is_aggregate_v<Site>, "a site is built by listing its fields");
    static_assert(K_SITE.line == 7U, "a site is a constant aggregate");
} // namespace
