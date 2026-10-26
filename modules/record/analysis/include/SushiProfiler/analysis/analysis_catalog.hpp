/**************************************************************************/
/* analysis_catalog.hpp                                                   */
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
 * @file analysis_catalog.hpp
 * @brief Declares the catalog that makes analyses by name.
 * @author Mustafa Garip
 * @see modules/record/analysis/README.md
 */

#pragma once

#include <SushiProfiler/analysis/analysis.hpp>
#include <SushiProfiler/version/export.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief Makes one analysis.
    using AnalysisFactory = std::unique_ptr<Analysis> (*)();

    /// @brief Maps analysis names to the factories that make them.
    class SUSHIPROFILER_API AnalysisCatalog final
    {
        public:
            /// @brief Adds @p factory under @p name; returns false when the name is taken.
            [[nodiscard]] bool add(std::string name, AnalysisFactory factory);

            /// @brief Returns a new analysis named @p name, or null when none is known.
            [[nodiscard]] std::unique_ptr<Analysis> make(std::string_view name) const;

            /// @brief Returns whether an analysis named @p name is known.
            [[nodiscard]] bool knows(std::string_view name) const noexcept;

            /// @brief Returns the known names in ascending order.
            [[nodiscard]] std::vector<std::string> names() const;

        private:
            /// @brief The entries, sorted by name.
            std::vector<std::pair<std::string, AnalysisFactory>> entries_;
    };

    /// @brief Adds every analysis SushiProfiler ships to @p catalog.
    SUSHIPROFILER_API void register_builtin_analyses(AnalysisCatalog& catalog);
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
