/**************************************************************************/
/* SushiProfiler.hpp                                                      */
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
 * @file SushiProfiler.hpp
 * @brief Includes every public header of SushiProfiler.
 * @author Mustafa Garip
 * @see modules/tools/umbrella/README.md
 */

#pragma once

#include <SushiProfiler/clock/calibration.hpp>
#include <SushiProfiler/clock/trace_clock.hpp>
#include <SushiProfiler/events/ids.hpp>
#include <SushiProfiler/events/trace_event.hpp>
#include <SushiProfiler/collector/drain_batch.hpp>
#include <SushiProfiler/collector/collector.hpp>
#include <SushiProfiler/collector/collector_settings.hpp>
#include <SushiProfiler/json/json_reader.hpp>
#include <SushiProfiler/json/json_value.hpp>
#include <SushiProfiler/json/json_writer.hpp>
#include <SushiProfiler/lock_wait/contended_lock.hpp>
#include <SushiProfiler/periods/period_timeline.hpp>
#include <SushiProfiler/rings/thread_ring.hpp>
#include <SushiProfiler/sites/name_table.hpp>
#include <SushiProfiler/sites/site.hpp>
#include <SushiProfiler/sites/site_registry.hpp>
#include <SushiProfiler/version/version.hpp>
#include <SushiProfiler/zones/admission.hpp>
#include <SushiProfiler/zones/macros.hpp>
#include <SushiProfiler/zones/records.hpp>
#include <SushiProfiler/zones/zone.hpp>
