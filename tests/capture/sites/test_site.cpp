/**************************************************************************/
/* test_site.cpp                                                          */
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
 * @file test_site.cpp
 * @brief Tests that a site is a constant aggregate and a handle starts unregistered.
 * @author Mustafa Garip
 * @see tests/capture/sites/README.md
 */

#include <SushiProfiler/sites/site.hpp>

#include <gtest/gtest.h>

using SushiProfiler::CounterOp;
using SushiProfiler::Site;
using SushiProfiler::SiteHandle;
using SushiProfiler::SiteRole;

namespace
{
    /// @brief A site built at compile time, as the recording macros build theirs.
    constexpr Site K_SITE{"frame", nullptr, "test_site.cpp", 7U, 1U, SiteRole::Zone,
                          CounterOp::Add, nullptr};
    static_assert(K_SITE.line == 7U, "a site is a constant aggregate");
} // namespace

TEST(Small_Site, AHandleStartsUnregistered)
{
    static SiteHandle handle{&K_SITE};

    EXPECT_EQ(handle.id.load(), 0U);
    EXPECT_EQ(handle.site, &K_SITE);
}
