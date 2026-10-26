/**************************************************************************/
/* record_file_writer.cpp                                                 */
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
 * @file record_file_writer.cpp
 * @brief Writes the record's JSON text to a file in binary mode, so the bytes match everywhere.
 * @author Mustafa Garip
 * @see modules/record/record_writers/README.md
 */

#include <SushiProfiler/record_writers/record_file_writer.hpp>

#include <fstream>
#include <utility>

#include <SushiProfiler/record_json/record_json.hpp>

namespace SushiProfiler
{
    RecordFileWriter::RecordFileWriter(std::string path) : path_(std::move(path))
    {

    }

    WriteStatus RecordFileWriter::write(const RunRecord& record, const std::vector<Table>& tables)
    {
        static_cast<void>(tables);
        std::ofstream file(path_, std::ios::binary | std::ios::trunc);
        if (!file)
            return WriteStatus::PathUnwritable;
        const std::string text = write_record(record);
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        file.close();
        return file ? WriteStatus::Written : WriteStatus::StreamFailed;
    }
} // namespace SushiProfiler
