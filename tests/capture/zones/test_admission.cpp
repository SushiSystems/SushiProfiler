/**************************************************************************/
/* test_admission.cpp                                                     */
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
 * @file test_admission.cpp
 * @brief Tests that a record is admitted only while the gate is open and its site enabled.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#include <SushiProfiler/zones/admission.hpp>

#include "recording_fixture.hpp"

#include <gtest/gtest.h>

using SushiProfiler::CounterOp;
using SushiProfiler::Site;
using SushiProfiler::SiteFilter;
using SushiProfiler::SiteHandle;
using SushiProfiler::SiteID;
using SushiProfiler::SiteRole;
using SushiProfiler::ThreadRing;

namespace
{
    /// @brief The admission tests, on one thread and a collector of their own.
    class Small_Admission : public SushiProfilerTests::OwnedRecordingFixture
    {
    };

    /// @brief A level-2 zone site in the group "Data".
    constexpr Site K_SITE{"admit", "Data", "test_admission.cpp", 1U, 2U, SiteRole::Zone,
                          CounterOp::Add, nullptr};
} // namespace

TEST_F(Small_Admission, AnEnabledSiteGetsTheThreadsRing)
{
    static SiteHandle handle{&K_SITE};
    SiteID site = 0U;

    ThreadRing* const ring = SushiProfiler::Detail::admit(collector, handle, site);

    EXPECT_NE(ring, nullptr);
    EXPECT_EQ(ring, collector.thread_ring());
}

TEST_F(Small_Admission, AnEnabledSiteGetsAnId)
{
    static SiteHandle handle{&K_SITE};
    SiteID site = 0U;

    static_cast<void>(SushiProfiler::Detail::admit(collector, handle, site));

    EXPECT_NE(site, 0U);
}

TEST_F(Small_Admission, AClosedCollectorAdmitsNothing)
{
    static SiteHandle handle{&K_SITE};
    SiteID site = 0U;
    collector.close();

    EXPECT_EQ(SushiProfiler::Detail::admit(collector, handle, site), nullptr);
}

TEST_F(Small_Admission, ADisabledSiteIsRefused)
{
    static SiteHandle handle{&K_SITE};
    SiteID site = 0U;
    SiteFilter filter;
    filter.level = 1U;
    collector.apply(filter);

    EXPECT_EQ(SushiProfiler::Detail::admit(collector, handle, site), nullptr);
}
