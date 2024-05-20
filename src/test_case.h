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

#ifndef MORIARTY_SRC_TEST_CASE_H_
#define MORIARTY_SRC_TEST_CASE_H_

#include <stdint.h>

#include <concepts>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/random_engine.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/mvariable.h"
#include "src/scenario.h"

namespace moriarty {

namespace moriarty_internal {
class TestCaseManager;  // Forward declaring the internal API
}  // namespace moriarty_internal

// TestCase
//
// A collection of variables representing a single test case. If you want to
// test your system with 5 inputs, there should be 5 `Case`s. See Moriarty and
// Generator documentation for more information.
class TestCase {
 public:
  // SetValue()
  //
  // Sets the variable `variable_name` to a specific `value`. The MVariable of
  // `variable_name` must match the MVariable at the global context.
  //
  // Example:
  //
  //     AddTestCase()
  //        .SetValue<MString>("X", "hello")
  //        .SetValue<MInteger>("Y", 3);
  //
  // The following are logically equivalent:
  //  * SetValue<MCustomType>("X", value);
  //  * ConstrainVariable("X", MCustomType().Is(value));
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  TestCase& SetValue(absl::string_view variable_name, T::value_type value);

  // ConstrainVariable()
  //
  // Adds extra constraints to `variable_name`. `constraints` will be
  // merged with the variable at global context.
  //
  // Examples:
  //
  //     // X is one of {10, 11, 12, ... , 20}.
  //     AddTestCase().ConstrainVariable("X", MInteger().Between(10, 20));
  //
  //     // X is one of {11, 13, 15}.
  //     AddTestCase()
  //         .ConstrainVariable("X", MInteger().Between(10, 15).IsOdd());
  //
  //     // Equivalent to above. X is one of {11, 13, 15}.
  //     AddTestCase()
  //         .ConstrainVariable("X", MInteger().Between(10, 15))
  //         .ConstrainVariable("X", MInteger().IsOdd());
  //
  //     // If "X" was `Between(1, 12)` in the global context, then it is one
  //     // of {10, 11, 12} now.
  //     AddTestCase().ConstrainVariable("X", MInteger().Between(10, 20));
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  TestCase& ConstrainVariable(absl::string_view variable_name, T constraints);

  // WithScenario()
  //
  // This TestCase is covering this `scenario`.
  TestCase& WithScenario(Scenario scenario);

 private:
  moriarty_internal::VariableSet variables_;
  std::vector<Scenario> scenarios_;

  // ---------------------------------------------------------------------------
  //    Start of Internal Extended API
  //
  // These functions can be accessed via `moriarty_internal::TestCaseManager`.
  // Users and Librarians should not need to access these functions. See
  // `TestCaseManager` for more details.
  friend class moriarty_internal::TestCaseManager;

  // SetVariables() [Internal Extended API]
  //
  // Sets the variables in the TestCase. This overwrites *all* previous
  // variables that were set in the test case.
  void SetVariables(moriarty_internal::VariableSet variables);

  // AssignAllValues() [Internal Extended API]
  //
  // Assigns the value of all variables in this test case, with all
  // randomization provided by `rng`.
  absl::StatusOr<moriarty_internal::ValueSet> AssignAllValues(
      moriarty_internal::RandomEngine& rng,
      std::optional<int64_t> approximate_generation_limit);

  // GetVariable() [Internal Extended API]
  //
  // Returns the variable named `variable_name`.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<T> GetVariable(absl::string_view variable_name) const;

  // ConstrainVariable() [Internal Extended API]
  //
  // Adds extra constraints to `variable_name`. `constraints` will be
  // merged with the variable at global context.
  //
  // Example:
  //
  //     AddTestCase().ConstrainVariable("X", MInteger().Between(10, 20));
  TestCase& ConstrainVariable(absl::string_view variable_name,
                              const moriarty_internal::AbstractVariable& var);

  //    End of Internal Extended API
  // ---------------------------------------------------------------------------

  // Distributes all known scenarios to the variable set.
  absl::Status DistributeScenarios();
};

// TestCaseMetadata
//
// Metadata about this test case. All 1-based.
//
// For example: 3 generators.
//  - A creates 4 test cases, run once (4 test cases total)
//  - B creates 3 test cases, run twice (6 test cases total)
//  - C creates 5 test cases, run three times (15 test cases total)
//
// Consider the 3rd test case that was added on the second run of C.
//
//  * GetTestCaseNumber() == 18 (4 + 2*3 + 5 + 3)
//  * GetGeneratedTestCaseMetadata().generator_name == "C"
//  * GetGeneratedTestCaseMetadata().generator_iteration == 2
//  * GetGeneratedTestCaseMetadata().case_number_in_generator = 3
class TestCaseMetadata {
 public:
  // Set the test case number for this TestCase. (1-based).
  TestCaseMetadata& SetTestCaseNumber(int test_case_number) {
    test_case_number_ = test_case_number;
    return *this;
  }

  // Which test case number is this? (1-based).
  [[nodiscard]] int GetTestCaseNumber() const { return test_case_number_; }

  // If the test case was generated, this is the important metadata that should
  // be considered. Fields may be added over time.
  struct GeneratedTestCaseMetadata {
    std::string generator_name;
    int generator_iteration;  // If this generator was called several times,
                              // which iteration?
    int case_number_in_generator;  // Which call of `AddTestCase()` was this?
                                   // (1-based)
  };
  TestCaseMetadata& SetGeneratorMetadata(
      GeneratedTestCaseMetadata generator_metadata) {
    generator_metadata_ = std::move(generator_metadata);
    return *this;
  }

  // If this TestCase was generated, return the metadata associated with it.
  const std::optional<GeneratedTestCaseMetadata>& GetGeneratorMetadata() const {
    return generator_metadata_;
  }

 private:
  int test_case_number_;
  std::optional<GeneratedTestCaseMetadata> generator_metadata_;
};

namespace moriarty_internal {

// TestCaseManager [Internal Extended API]
//
// Contains functions that are public with respect to `TestCase`, but should be
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
// See corresponding functions in `TestCase` for documentation.
//
// Example usage:
//
//   TestCase t;
//   moriarty::moriarty_internal::TestCaseManager(&t).SetVariables( ... );
class TestCaseManager {
 public:
  // This class does not take ownership of `test_case_to_manage`
  explicit TestCaseManager(moriarty::TestCase* test_case_to_manage);

  void SetVariables(VariableSet variables);
  absl::StatusOr<ValueSet> AssignAllValues(
      moriarty_internal::RandomEngine& rng,
      std::optional<int64_t> approximate_generation_limit);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<T> GetVariable(absl::string_view variable_name) const;
  TestCase& ConstrainVariable(absl::string_view variable_name,
                              const moriarty_internal::AbstractVariable& var);

 private:
  TestCase& managed_test_case_;  // Not owned by this class
};

}  // namespace moriarty_internal

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
TestCase& TestCase::SetValue(absl::string_view variable_name,
                             T::value_type value) {
  ConstrainVariable(variable_name, T().Is(std::move(value)));
  return *this;
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
TestCase& TestCase::ConstrainVariable(absl::string_view variable_name,
                                      T constraints) {
  ABSL_CHECK_OK(
      variables_.AddOrMergeVariable(variable_name, std::move(constraints)));
  return *this;
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<T> TestCase::GetVariable(absl::string_view variable_name) const {
  return variables_.GetVariable<T>(variable_name);
}

namespace moriarty_internal {

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<T> TestCaseManager::GetVariable(
    absl::string_view variable_name) const {
  return managed_test_case_.GetVariable<T>(variable_name);
}

}  // namespace moriarty_internal

}  // namespace moriarty

#endif  // MORIARTY_SRC_TEST_CASE_H_
