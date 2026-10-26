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
 * @file test_admission.cpp
 * @brief Tests that a record is admitted only while the gate is open and its site enabled.
 * @author Mustafa Garip
 * @see tests/capture/zones/README.md
 */

#include <SushiProfiler/zones/admission.hpp>

#include "recording_fixture.hpp"

#include <gtest/gtest.h>

using SushiProfiler::Collector;
using SushiProfiler::CounterOp;
using SushiProfiler::Site;
using SushiProfiler::SiteFilter;
using SushiProfiler::SiteHandle;
using SushiProfiler::SiteID;
using SushiProfiler::SiteRole;

namespace
{
    /// @brief The admission tests, on one thread.
    class Small_Admission : public SushiProfilerTests::RecordingFixture
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

    EXPECT_EQ(SushiProfiler::Detail::admit(handle, site), Collector::ring());
    EXPECT_NE(site, 0U);
}

TEST_F(Small_Admission, AClosedCollectorAdmitsNothing)
{
    static SiteHandle handle{&K_SITE};
    SiteID site = 0U;
    Collector::instance().close();

    EXPECT_EQ(SushiProfiler::Detail::admit(handle, site), nullptr);
}

TEST_F(Small_Admission, ADisabledSiteIsRefused)
{
    static SiteHandle handle{&K_SITE};
    SiteID site = 0U;
    SiteFilter filter;
    filter.level = 1U;
    Collector::instance().apply(filter);

    EXPECT_EQ(SushiProfiler::Detail::admit(handle, site), nullptr);
}
