// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/simple_io.h"

#include <istream>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "src/exporter.h"
#include "src/importer.h"
#include "src/internal/abstract_variable.h"
#include "src/librarian/io_config.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

// -----------------------------------------------------------------------------
//  SimpleIO

const std::vector<SimpleIO::Line>& SimpleIO::LinesInHeader() const {
  return lines_in_header_;
}

const std::vector<SimpleIO::Line>& SimpleIO::LinesPerTestCase() const {
  return lines_per_test_case_;
}

const std::vector<SimpleIO::Line>& SimpleIO::LinesInFooter() const {
  return lines_in_footer_;
}

SimpleIO& SimpleIO::WithNumberOfTestCasesInHeader() {
  has_number_of_test_cases_in_header_ = true;
  return *this;
}

bool SimpleIO::HasNumberOfTestCasesInHeader() const {
  return has_number_of_test_cases_in_header_;
}

SimpleIOExporter SimpleIO::Exporter(std::ostream& os) const {
  return SimpleIOExporter(*this, os);
}

SimpleIOImporter SimpleIO::Importer(std::istream& is) const {
  return SimpleIOImporter(*this, is);
}

// -----------------------------------------------------------------------------
//  SimpleIOExporter

SimpleIOExporter::SimpleIOExporter(SimpleIO simple_io, std::ostream& os)
    : simple_io_(std::move(simple_io)) {
  io_config_.SetOutputStream(os);
  SetIOConfig(&io_config_);
}

void SimpleIOExporter::StartExport() {
  if (simple_io_.HasNumberOfTestCasesInHeader()) {
    ABSL_CHECK_OK(io_config_.PrintToken(absl::StrCat(NumTestCases())));
    ABSL_CHECK_OK(io_config_.PrintWhitespace(Whitespace::kNewline));
  }

  PrintLines(simple_io_.LinesInHeader());
}

void SimpleIOExporter::ExportTestCase() {
  PrintLines(simple_io_.LinesPerTestCase());
}

void SimpleIOExporter::EndExport() { PrintLines(simple_io_.LinesInFooter()); }

void SimpleIOExporter::PrintLines(absl::Span<const SimpleIO::Line> lines) {
  for (const SimpleIO::Line& line : lines) PrintLine(line);
}

void SimpleIOExporter::PrintLine(const SimpleIO::Line& line) {
  for (int i = 0; i < line.size(); i++) {
    if (i != 0) ABSL_CHECK_OK(io_config_.PrintWhitespace(Whitespace::kSpace));
    PrintToken(line[i]);
  }
  ABSL_CHECK_OK(io_config_.PrintWhitespace(Whitespace::kNewline));
}

void SimpleIOExporter::PrintVariable(absl::string_view variable_name) {
  absl::StatusOr<moriarty_internal::AbstractVariable*> var =
      moriarty_internal::ExporterManager(this).GetAbstractVariable(
          variable_name);
  ABSL_CHECK_OK(var) << "Unknown variable name: " << variable_name;
  ABSL_CHECK_OK((*var)->PrintValue());
}

void SimpleIOExporter::PrintToken(const SimpleIOToken& token) {
  if (std::holds_alternative<std::string>(token))
    PrintVariable(std::get<std::string>(token));
  else
    ABSL_CHECK_OK(
        io_config_.PrintToken(std::string(std::get<StringLiteral>(token))));
}

// -----------------------------------------------------------------------------
//  SimpleIOImporter

SimpleIOImporter::SimpleIOImporter(SimpleIO simple_io, std::istream& is)
    : simple_io_(std::move(simple_io)) {
  io_config_.SetInputStream(is);
  SetIOConfig(&io_config_);
}

void SimpleIOImporter::SetNumTestCases(int num_test_cases) {
  num_test_cases_ = num_test_cases;
}

absl::Status SimpleIOImporter::StartImport() {
  if (simple_io_.HasNumberOfTestCasesInHeader()) {
    MORIARTY_ASSIGN_OR_RETURN(std::string num_cases_str, io_config_.ReadToken(),
                              _ << "Unable to read number of cases.");
    int num_test_cases = 0;
    if (!absl::SimpleAtoi(num_cases_str, &num_test_cases)) {
      return absl::InvalidArgumentError(absl::Substitute(
          "Unable to parse number of test cases: $0", num_cases_str));
    }
    if (num_test_cases < 0) {
      return absl::InvalidArgumentError(absl::Substitute(
          "Number of test cases must be nonnegative, but got: $0",
          num_test_cases));
    }
    SetNumTestCases(num_test_cases);
    MORIARTY_RETURN_IF_ERROR(io_config_.ReadWhitespace(Whitespace::kNewline));
  }

  MORIARTY_RETURN_IF_ERROR(ReadLines(simple_io_.LinesInHeader()));
  Importer::SetNumTestCases(num_test_cases_);
  return absl::OkStatus();
}

absl::Status SimpleIOImporter::ImportTestCase() {
  return ReadLines(simple_io_.LinesPerTestCase());
}

absl::Status SimpleIOImporter::EndImport() {
  return ReadLines(simple_io_.LinesInFooter());
}

absl::Status SimpleIOImporter::ReadLines(
    absl::Span<const SimpleIO::Line> lines) {
  for (const SimpleIO::Line& line : lines) {
    MORIARTY_RETURN_IF_ERROR(ReadLine(line));
  }
  return absl::OkStatus();
}

absl::Status SimpleIOImporter::ReadLine(const SimpleIO::Line& line) {
  for (int i = 0; i < line.size(); i++) {
    if (i != 0) {
      MORIARTY_RETURN_IF_ERROR(io_config_.ReadWhitespace(Whitespace::kSpace));
    }
    MORIARTY_RETURN_IF_ERROR(ReadToken(line[i]));
  }
  return io_config_.ReadWhitespace(Whitespace::kNewline);
}

absl::Status SimpleIOImporter::ReadVariable(absl::string_view variable_name) {
  MORIARTY_ASSIGN_OR_RETURN(
      moriarty_internal::AbstractVariable * var,
      moriarty_internal::ImporterManager(this).GetAbstractVariable(
          variable_name),
      _ << "Unknown variable name: " << variable_name);
  return var->ReadValue();
}

absl::Status SimpleIOImporter::ReadToken(const SimpleIOToken& token) {
  if (std::holds_alternative<std::string>(token)) {
    return ReadVariable(std::get<std::string>(token));
  }

  MORIARTY_ASSIGN_OR_RETURN(std::string read_token, io_config_.ReadToken());
  std::string expected = std::string(std::get<StringLiteral>(token));
  if (read_token != expected) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Expected to read '$0', but read '$1' instead.", expected, read_token));
  }
  return absl::OkStatus();
}

}  // namespace moriarty
