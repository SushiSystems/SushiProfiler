/**************************************************************************/
/* record_file_writer.hpp                                                 */
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
 * @file record_file_writer.hpp
 * @brief Declares the writer that saves the record as a JSON file.
 * @author Mustafa Garip
 * @see modules/record/record_writers/README.md
 */

#pragma once

#include <SushiProfiler/record_writers/record_writer.hpp>
#include <SushiProfiler/version/export.hpp>

#include <string>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiProfiler
{
    /// @brief Saves the record to one file in the record format; the tables are not saved.
    class SUSHIPROFILER_API RecordFileWriter final : public RecordWriter
    {
        public:
            /// @brief Builds a writer to @p path, replaced on each write.
            explicit RecordFileWriter(std::string path);

            /// @brief Writes @p record to the path.
            [[nodiscard]] WriteStatus write(const RunRecord& record,
                                            const std::vector<Table>& tables) override;

        private:
            /// @brief Where the record goes.
            std::string path_;
    };
} // namespace SushiProfiler
SUSHIPROFILER_DLL_WARNINGS_POP
