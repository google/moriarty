/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MORIARTY_SRC_EXPORTER_H_
#define MORIARTY_SRC_EXPORTER_H_

#include <concepts>
#include <optional>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/status_utils.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/test_case.h"

namespace moriarty {

namespace moriarty_internal {
class ExporterManager;  // Forward declaring the internal API
}  // namespace moriarty_internal

// Exporter [Abstract Class]
//
// Provides an `Exporter` interface for Moriarty. There are only a few public
// functions available. An Exporter is meant to be used inside the Moriarty
// ecosystem. Use `Moriarty::ExportTestCases(YourExporter())` to use your
// exporter.
class Exporter {
 public:
  virtual ~Exporter() = default;

  // ExportTestCases()
  //
  // Exports a collection of TestCases.
  //
  // 1. `StartExport()` is called exactly once.
  // 2. `ExportTestCase()` and `TestCaseDivider()` are called N and N-1 times,
  //    respectively. Once for each test case and once between each pair of test
  //    cases, respectively.
  // 3. `EndExport()` is called exactly once.
  void ExportTestCases();

 protected:
  // StartExport() [virtual/optional]
  //
  // User-written code to export information prior to the first test case.
  // Any call to `GetValue` will crash.
  //
  // By default, this does nothing.
  virtual void StartExport() {}

  // ExportTestCase() [pure virtual]
  //
  // User-written code to export a single test case in whatever way they see
  // fit. For example, they can print everything to a file or database, etc.
  virtual void ExportTestCase() = 0;

  // TestCaseDivider() [virtual/optional]
  //
  // User-written code to some work between two test cases (read
  // whitespace, refresh caches, etc). No information about the test cases
  // should be in this function. Any call to `GetValue` will crash.
  //
  // By default, this does nothing.
  virtual void TestCaseDivider() {}

  // EndExport() [virtual/optional]
  //
  // User-written code to import information after the final test case. Any call
  // to `GetValue` will crash.
  //
  // By default, this does nothing.
  virtual void EndExport() {}

  // GetValue<>()
  //
  // Grabs the variable with the specified name. The argument is the type it is
  // returned as.
  //
  //   int64_t N = GetValue<MInteger>("N");
  //
  // Crashes on failure. See `TryGetValue()` for non-crashing version.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  T::value_type GetValue(absl::string_view variable_name) const;

  // TryGetValue<>()
  //
  // Grabs the variable with the specified name. The argument is the type it is
  // returned as.
  //
  //   absl::StatusOr<int64_t> N = TryGetValue<MInteger>("N");
  //
  // Returns status on failure. See `GetValue()` for simpler API version.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> TryGetValue(
      absl::string_view variable_name) const;

  // GetTestCaseMetadata()
  //
  // Retrieves the metadata for this test case.
  //
  // Crashes on failure. See `TryGetTestCaseMetadata()` for non-crashing
  // version.
  [[nodiscard]] TestCaseMetadata GetTestCaseMetadata() const;

  // TryGetTestCaseMetadata()
  //
  // Retrieves the metadata for this test case.
  //
  // Returns status on failure. See `GetTestCaseMetadata()` for simpler API
  // version.
  absl::StatusOr<TestCaseMetadata> TryGetTestCaseMetadata() const;

  // NumTestCases()
  //
  // Returns the number of test cases to be exported.
  [[nodiscard]] int NumTestCases() const;

  // SetIOConfig()
  //
  // Sets the internal IOConfig to be passed to all variables.
  void SetIOConfig(librarian::IOConfig* io_config);

  // TODO(darcybest): Add `Print()`

 private:
  std::vector<moriarty_internal::ValueSet> all_values_;
  std::vector<TestCaseMetadata> all_metadata_;

  std::optional<moriarty_internal::ValueSet> current_values_;
  std::optional<TestCaseMetadata> current_metadata_;

  moriarty_internal::VariableSet general_constraints_;
  librarian::IOConfig* io_config_ = nullptr;

  // ---------------------------------------------------------------------------
  //    Start of Internal Extended API
  //
  // These functions can be accessed via `moriarty_internal::ExporterManager`.
  // Users and Librarians should not need to access these functions. See
  // `ExporterManager` for more details.
  friend class moriarty_internal::ExporterManager;

  // SetAllValues() [Internal Extended API]
  //
  // Sets the `values` that are to be exported. They will be exported in the
  // order provided here. If metadata is needed, the order of `ValueSet`s must
  // match the order in `SetTestCaseMetadata`.
  void SetAllValues(std::vector<moriarty_internal::ValueSet> values);

  // SetTestCaseMetadata() [Internal Extended API]
  //
  // Sets the metadata for the `TestCase`s that will be exported. The order of
  // these must match the size and order of `ValueSet`s in `SetAllValues`. This
  // must be called after `SetAllValues()`.
  void SetTestCaseMetadata(std::vector<TestCaseMetadata> metadata);

  // GetAbstractVariable() [Internal Extended API]
  //
  // Returns a pointer to variable `variable_name` from the general constraints.
  // Ownership is *not* transferred to the caller.
  absl::StatusOr<moriarty_internal::AbstractVariable*> GetAbstractVariable(
      absl::string_view variable_name);

  // SetGeneralConstraints() [Internal Extended API]
  //
  // Sets the general constraints.
  void SetGeneralConstraints(
      moriarty_internal::VariableSet general_constraints);

  // GetIOConfig() [Internal Extended API]
  //
  // Returns the IOConfig if it has been set. `nullptr` otherwise.
  [[nodiscard]] librarian::IOConfig* GetIOConfig();

  //    End of Internal Extended API
  // ---------------------------------------------------------------------------
};

namespace moriarty_internal {

// ExporterManager [Internal Extended API]
//
// Contains functions that are public with respect to `Exporter`, but should be
// considered `private` with respect to users of Moriarty. (Effectively Java's
// package-private.)
//
// Users and Librarians of Moriarty should not need any of these functions, and
// the base functionality of Moriarty assumes these functions are not called.
//
// One potential exception is if you are creating very generic Moriarty code and
// need access to the underlying `moriarty_internal::AbstractVariable` pointer.
// This should be extremely rare and is at your own risk.
//
// See corresponding functions in `Exporter` for documentation.
//
// Example usage:
//   class ExampleExporter : public Exporter { ... };
//
//   ExampleExporter e;
//   moriarty_internal::ExporterManager(&e).SetSeed({1, 2, 3});
class ExporterManager {
 public:
  // This class does not take ownership of `exporter_to_manage`
  explicit ExporterManager(moriarty::Exporter* exporter_to_manage);

  void SetAllValues(std::vector<ValueSet> values);
  void SetTestCaseMetadata(std::vector<TestCaseMetadata> metadata);
  absl::StatusOr<AbstractVariable*> GetAbstractVariable(
      absl::string_view variable_name);
  void SetGeneralConstraints(VariableSet general_constraints);
  librarian::IOConfig* GetIOConfig();

 private:
  moriarty::Exporter& managed_exporter_;
};

}  // namespace moriarty_internal

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
T::value_type Exporter::GetValue(absl::string_view variable_name) const {
  return moriarty_internal::TryFunctionOrCrash<typename T::value_type>(
      [&, this]() { return this->TryGetValue<T>(variable_name); }, "GetValue");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Exporter::TryGetValue(
    absl::string_view variable_name) const {
  if (!current_values_) {
    // You can also get this error if you are calling `TryGetValue()` from
    // `StartExport`, `TestCaseDivider`, `EndExport`.
    return MisconfiguredError("Exporter", "TryGetValue",
                              InternalConfigurationType::kValueSet);
  }

  return current_values_->Get<T>(variable_name);
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_EXPORTER_H_
