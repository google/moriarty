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

#ifndef MORIARTY_SRC_IMPORTER_H_
#define MORIARTY_SRC_IMPORTER_H_

#include <concepts>
#include <optional>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/analysis_bootstrap.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"

namespace moriarty {

namespace moriarty_internal {
class ImporterManager;  // Forward declaring the internal API
}  // namespace moriarty_internal

// Importer [Abstract Class]
//
// Provides an `Importer` interface for Moriarty. An Importer is meant to be
// used inside the Moriarty ecosystem. Use
// `Moriarty::ImportTestCases(YourImporter())` to use your importer.
class Importer {
 public:
  virtual ~Importer() = default;

  // ImportTestCases()
  //
  // Imports a collection of TestCases.
  //
  // 1. `StartImport()` is called exactly once.
  // 2. `ImportTestCase()` and `TestCaseDivider()` are called several times.
  // 3. `EndImport()` is called exactly once.
  //
  // If `SetNumTestCases(N)` is called in `StartImport()`:
  //  * `ImportTestCase()` will be called exactly N times
  //  * `TestCaseDivider()` will be called exactly N-1 times (interleaved
  //     between the `ImportTestCase()` calls).
  //
  // If `SetNumTestCases(N)` is *not* called in `StartImport()`:
  //  * `ImportTestCase()` and `TestCaseDivider()` are called until `Done()` is
  //    called. See `Done()` for more information.
  absl::Status ImportTestCases();

 protected:
  // StartImport() [virtual/optional]
  //
  // User-written code to import information prior to the first test case.
  // All calls to `SetValue()` will be ignored.
  //
  // By default, this does nothing.
  virtual absl::Status StartImport() { return absl::OkStatus(); }

  // ImportTestCase() [pure virtual]
  //
  // User-written code to import a single test case in whatever way they see
  // fit. For example, they can read everything from a file or a database, etc.
  //
  // If you use `SetNumTestCases(N)`, `ImportTestCase()` will be called `N`
  // times. Otherwise, the user should call `Done()` and return. The case where
  // `Done()` is called will be discarded.
  virtual absl::Status ImportTestCase() = 0;

  // TestCaseDivider() [virtual/optional]
  //
  // User-written code to some work between two test cases (read
  // whitespace, refresh caches, etc). No information about the test cases
  // should be in this function. For example, all calls to `SetValue()`,
  // `ReadValue()`, etc. will be ignored.
  //
  // If `SetNumTestCases()` is used and there are N test cases, this will be
  // called N-1 times.  If `SetNumTestCases()` is not used, this will be called
  // after each test case.
  //
  // By default, this does nothing.
  virtual absl::Status TestCaseDivider() { return absl::OkStatus(); }

  // EndImport() [virtual/optional]
  //
  // User-written code to import information after the final test case. All
  // calls to `SetValue()` will be ignored.
  //
  // By default, this does nothing.
  virtual absl::Status EndImport() { return absl::OkStatus(); }

  // Done()
  //
  // Call `Done()` from within `ImportTestCase()` or `TestCaseDivider()` to
  // indicate that you are finished importing. You must not have called
  // `SetNumTestCases()` in `StartImport()`.
  //
  // If called from within `ImportTestCase()`, the current case you are
  // importing will be discarded. If called from within `TestCaseDivider()`, the
  // most recent test case will *not* be discarded. In either case, importing
  // will stop.
  //
  // `IsDone()` is only checked after your function has completed, `Done()` does
  // not stop your current function, just sets a flag. So you likely your code
  // reads `Done(); return;`.
  void Done();

  // IsDone()
  //
  // Determines if `Done()` has been called.
  [[nodiscard]] bool IsDone() const;

  // SetNumTestCases()
  //
  // Sets the number of test cases to read. Should only be called from within
  // `StartImport()`. Calling this after `StartImport()` has no effect. If you
  // call this, you may not use `Done()`.
  void SetNumTestCases(int num_test_cases);

  // SetValue()
  //
  // Sets the variable `variable_name` to a specific `value`. The MVariable of
  // this variable must match the MVariable at the global context.
  //
  // Example usage:
  //   SetValue<MInteger>("N", 10);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  void SetValue(absl::string_view variable_name, T::value_type value);

  // TestCaseMetadata
  //
  // Metadata about this test case. All 1-based.
  struct TestCaseMetadata {
    int test_case_number;    // 1-based
    int num_test_cases = 0;  // == 0 if `SetNumTestCases` was not called.
  };

  // GetTestCaseMetadata()
  //
  // Metadata about the current test case. In the metadata, `test_case_number`
  // is updated just before `ImportTestCase()` (just after `TestCaseDivider()`).
  //
  // The metadata is only up to date while an active import is occurring.
  // Calling this in any function other than during `ImportTestCase()` and
  // `TestCaseDivider()` will give undefined results.
  [[nodiscard]] TestCaseMetadata GetTestCaseMetadata() const;

  // SatisfiesConstraints()
  //
  // Checks if `value` satisfies the constraints in `constraints`. Returns a
  // non-ok status with the reason if not. You may use other known variables in
  // your constraints. Examples:
  //
  //   absl::Status s = SatisfiesConstraints(MInteger().AtMost("N"), 25);
  //   absl::Status s = SatisfiesConstraints(MString().OfLength(5), "hello");
  //
  // This should only be called from within `ImportTestCase()`.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::Status SatisfiesConstraints(T constraints,
                                    const T::value_type& value) const;

  // SetIOConfig()
  //
  // Sets the internal IOConfig to be passed to all variables.
  // TODO(darcybest): Hide this implementation detail. We should have
  // `SetInputStream()` instead so they don't need to know about `IOConfig`.
  void SetIOConfig(librarian::IOConfig* io_config);

  // TODO(darcybest): Add `Read()`

 private:
  std::vector<moriarty_internal::ValueSet> test_cases_;
  moriarty_internal::ValueSet current_test_case_;
  moriarty_internal::VariableSet general_constraints_;
  TestCaseMetadata metadata_;
  std::optional<int> num_test_cases_;
  bool done_ = false;

  librarian::IOConfig* io_config_ = nullptr;

  // ---------------------------------------------------------------------------
  //    Start of Internal Extended API
  //
  // These functions can be accessed via `moriarty_internal::ImporterManager`.
  // Users and Librarians should not need to access these functions. See
  // `ImporterManager` for more details.
  friend class moriarty_internal::ImporterManager;

  // SetGeneralConstraints() [Internal Extended API]
  //
  // Sets the general constraints.
  void SetGeneralConstraints(
      moriarty_internal::VariableSet general_constraints);

  // GetTestCases() [Internal Extended API]
  //
  // Returns the list of test cases which have been set.
  std::vector<moriarty_internal::ValueSet> GetTestCases() const;

  // GetCurrentTestCase() [Internal Extended API]
  //
  // Returns the current test case.
  const moriarty_internal::ValueSet& GetCurrentTestCase() const;

  // GetIOConfig() [Internal Extended API]
  //
  // Returns the IOConfig if it has been set. `nullptr` otherwise.
  librarian::IOConfig* GetIOConfig();

  // GetAbstractVariable() [Internal Extended API]
  //
  // Returns the AbstractVariable with the appropriate name.
  absl::StatusOr<moriarty_internal::AbstractVariable*> GetAbstractVariable(
      absl::string_view variable_name);

  //    End of Internal Extended API
  // ---------------------------------------------------------------------------

  // Helper functions to import when the number of cases is known/unknown.
  absl::Status ImportTestCasesWithKnownNumberOfCases();
  absl::Status ImportTestCasesWithUnknownNumberOfCases();
};

namespace moriarty_internal {

// ImporterManager [Internal Extended API]
//
// Contains functions that are public with respect to `Importer`, but should be
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
// See corresponding functions in `Importer` for documentation.
//
// Example usage:
//   class ExampleImporter : public moriarty::Importer { ... };
//
//   ExampleImporter e;
//   moriarty::moriarty_internal::ImporterManager(&e).SetGeneralConstraints( ...
//   );
class ImporterManager {
 public:
  // This class does not take ownership of `importer_to_manage`
  explicit ImporterManager(moriarty::Importer* importer_to_manage);

  void SetGeneralConstraints(VariableSet general_constraints);
  std::vector<moriarty_internal::ValueSet> GetTestCases() const;
  absl::StatusOr<AbstractVariable*> GetAbstractVariable(
      absl::string_view variable_name);
  const moriarty_internal::ValueSet& GetCurrentTestCase() const;
  librarian::IOConfig* GetIOConfig();

 private:
  moriarty::Importer& managed_importer_;
};

}  // namespace moriarty_internal

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
void Importer::SetValue(absl::string_view variable_name, T::value_type value) {
  current_test_case_.Set<T>(variable_name, value);
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::Status Importer::SatisfiesConstraints(T constraints,
                                            const T::value_type& value) const {
  return moriarty_internal::SatisfiesConstraints(
      std::move(constraints), value,
      /* known_values = */ current_test_case_,
      /* variables = */ general_constraints_,
      "Generator::SatisfiesConstraints");
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_IMPORTER_H_
