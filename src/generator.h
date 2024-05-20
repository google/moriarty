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

#ifndef MORIARTY_SRC_GENERATOR_H_
#define MORIARTY_SRC_GENERATOR_H_

#include <stdint.h>

#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "src/errors.h"
#include "src/internal/analysis_bootstrap.h"
#include "src/internal/generation_config.h"
#include "src/internal/random_config.h"
#include "src/internal/random_engine.h"
#include "src/internal/status_utils.h"
#include "src/internal/universe.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/mvariable.h"
#include "src/property.h"
#include "src/scenario.h"
#include "src/test_case.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

namespace moriarty_internal {
class GeneratorManager;  // Forward declaring the internal API
}  // namespace moriarty_internal

// Generator
//
// A class to be derived from to create a Moriarty Generator. You must override
// the `GenerateTestCases()` function, which should call `AddTestCase()`  once
// or several times. Then add the Generator to Moriarty via `AddGenerator()`.
class Generator {
 public:
  virtual ~Generator() = default;

  // AddTestCase()
  //
  // Creates an empty case (no variables set) and returns a reference to that
  // case.
  //
  // Crashes on failure. See `TryAddTestCase()` for a non-crashing version.
  TestCase& AddTestCase();

  // TryAddTestCase()
  //
  // Creates an empty case (no variables set) and returns a pointer to that
  // case.
  //
  // Returns status on failure. See `AddTestCase()` for simpler API version.
  absl::StatusOr<absl::Nonnull<TestCase*>> TryAddTestCase();

  // AddOptionalTestCase()
  //
  // Creates an empty case (no variables set) and returns a reference to that
  // case. If there is an error while generating variables in this test case,
  // then this test case will be ignored.
  //
  // Crashes on failure. See `TryAddOptionalTestCase()` for a non-crashing
  // version.
  TestCase& AddOptionalTestCase();

  // TryAddOptionalTestCase()
  //
  // Creates an empty case (no variables set) and returns a reference to that
  // case. If there is an error while generating variables in this test case,
  // then this test case will be ignored.
  //
  // Returns status on failure. See `AddOptionalTestCase()` for simpler API
  // version.
  absl::StatusOr<absl::Nonnull<TestCase*>> TryAddOptionalTestCase();

  // WithScenario()
  //
  // All future calls to `AddTestCase()` will have this scenario added into it.
  // All previous calls to `AddTestCase()` will not be affected.
  //
  // Crashes on failure. See `TryWithScenario()` for a non-crashing version.
  void WithScenario(Scenario scenario);

  // TryWithScenario()
  //
  // All future calls to `AddTestCase()` will have this scenario added into it.
  // All previous calls to `AddTestCase()` will not be affected.
  //
  // Returns status on failure. See `WithScenario()` for simpler API version.
  absl::Status TryWithScenario(Scenario scenario);

  // GenerateTestCases() [virtual]
  //
  // You must override this function in your derived Generator. This
  // GenerateTestCases() function will be called and any values added via
  // AddCase() will be saved.
  virtual void GenerateTestCases() = 0;

 protected:
  // Random()
  //
  // Generates a random value that is described by some MVariable `m`.
  //
  // Example usage:
  //    int x = Random(MInteger().Between(1, 10));
  //
  // Crashes on failure. See `TryRandom()` for a non-crashing version.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  [[nodiscard]] T::value_type Random(T m);

  // TryRandom()
  //
  // Generates a random value that is described by `m`.
  //
  // Example usage:
  //    absl::StatusOr<int64_t> x = TryRandom(MInteger().Between(1, 10));
  //
  // Returns status on failure. See `Random()` for simpler API version.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> TryRandom(T m);

  // RandomInteger()
  //
  // Returns a random integer in the closed interval [min, max].
  //
  // Crashes on failure. See `TryRandomInteger()` for a non-crashing version.
  [[nodiscard]] int64_t RandomInteger(int64_t min, int64_t max);

  // TryRandomInteger()
  //
  // Returns a random integer in the closed interval [min, max].
  //
  // Returns status on failure. See `RandomInteger()` for simpler API version.
  absl::StatusOr<int64_t> TryRandomInteger(int64_t min, int64_t max);

  // RandomInteger()
  //
  // Returns a random integer in the semi-closed interval [0, n). Useful for
  // random indices.
  //
  // Crashes on failure. See `TryRandomInteger()` for a non-crashing version.
  [[nodiscard]] int64_t RandomInteger(int64_t n);

  // TryRandomInteger()
  //
  // Returns a random integer in the semi-closed interval [0, n). Useful for
  // random indices.
  //
  // Returns status on failure. See `RandomInteger()` for simpler API version.
  absl::StatusOr<int64_t> TryRandomInteger(int64_t n);

  // Shuffle()
  //
  // Shuffles the elements in `container`.
  //
  // Crashes on failure. See `TryShuffle()` for a non-crashing version.
  template <typename T>
  void Shuffle(std::vector<T>& container);

  // TryShuffle()
  //
  // Shuffles the elements in `container`.
  //
  // Returns status on failure. See `Shuffle()` for simpler API version.
  template <typename T>
  absl::Status TryShuffle(std::vector<T>& container);

  // RandomElement()
  //
  // Returns a random element of `container`.
  //
  // Crashes on failure. See `TryRandomElement()` for a non-crashing version.
  template <typename T>
  [[nodiscard]] T RandomElement(const std::vector<T>& container);

  // TryRandomElement()
  //
  // Returns a random element of `container`.
  //
  // Returns status on failure. See `RandomElement()` for simpler API version.
  template <typename T>
  absl::StatusOr<T> TryRandomElement(const std::vector<T>& container);

  // RandomElementsWithReplacement()
  //
  // Returns k (randomly ordered) elements of `container`, possibly with
  // duplicates.
  //
  // Crashes on failure. See `TryRandomElementsWithReplacement()` for a
  // non-crashing version.
  template <typename T>
  [[nodiscard]] std::vector<T> RandomElementsWithReplacement(
      const std::vector<T>& container, int k);

  // TryRandomElementsWithReplacement()
  //
  // Returns k (randomly ordered) elements of `container`, possibly with
  // duplicates.
  //
  // Returns status on failure. See `RandomElementsWithReplacement()` for
  // simpler API version.
  template <typename T>
  absl::StatusOr<std::vector<T>> TryRandomElementsWithReplacement(
      const std::vector<T>& container, int k);

  // RandomElementsWithoutReplacement()
  //
  // Returns k (randomly ordered) elements of `container`, without duplicates.
  //
  // Each element may appear at most once. Note that if there are duplicates in
  // `container`, each of those could be returned once each.
  //
  // RandomElementsWithoutReplacement({0, 1, 1, 1}, 2) could return {1, 1}.
  //
  // Crashes on failure. See `TryRandomElementsWithoutReplacement()` for a
  // non-crashing version.
  template <typename T>
  [[nodiscard]] std::vector<T> RandomElementsWithoutReplacement(
      const std::vector<T>& container, int k);

  // TryRandomElementsWithoutReplacement()
  //
  // Returns k (randomly ordered) elements of `container`, without duplicates.
  //
  // Each element may appear at most once. Note that if there are duplicates in
  // `container`, each of those could be returned once each.
  //
  // So TryRandomElementsWithoutReplacement({0, 1, 1, 1}, 2) could return {1,
  // 1}.
  //
  // Returns status on failure. See `RandomElementsWithoutReplacement()` for
  // simpler API version.
  template <typename T>
  absl::StatusOr<std::vector<T>> TryRandomElementsWithoutReplacement(
      const std::vector<T>& container, int k);

  // RandomPermutation()
  //
  // Returns a random permutation of {0, 1, ... , n-1}.
  //
  // Crashes on failure. See `TryRandomPermutation()` for a non-crashing
  // version.
  [[nodiscard]] std::vector<int> RandomPermutation(int n);

  // TryRandomPermutation()
  //
  // Returns a random permutation of {0, 1, ... , n-1}.
  //
  // Returns status on failure. See `RandomPermutation()` for simpler API
  // version.
  absl::StatusOr<std::vector<int>> TryRandomPermutation(int n);

  // RandomPermutation()
  //
  // Returns a random permutation of {min, min + 1, ... , min + (n-1)}.
  //
  // Requires min + (n-1) to not overflow T.
  //
  // Crashes on failure. See `TryRandomPermutation()` for a non-crashing
  // version.
  template <typename T>
    requires std::integral<T>
  [[nodiscard]] std::vector<T> RandomPermutation(int n, T min);

  // TryRandomPermutation()
  //
  // Returns a random permutation of {min, min + 1, ... , min + (n-1)}.
  //
  // Requires min + (n-1) to not overflow T.
  //
  // Returns status on failure. See `RandomPermutation()` for simpler API
  // version.
  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> TryRandomPermutation(int n, T min);

  // DistinctIntegers()
  //
  // Returns k (randomly ordered) distinct integers from
  // {min, min + 1, ... , min + (n-1)}.
  //
  // Requires min + (n-1) to not overflow T.
  //
  // Crashes on failure. See `TryDistinctIntegers()` for a non-crashing version.
  template <typename T>
    requires std::integral<T>
  [[nodiscard]] std::vector<T> DistinctIntegers(T n, int k, T min = 0);

  // TryDistinctIntegers()
  //
  // Returns k (randomly ordered) distinct integers from
  // {min, min + 1, ... , min + (n-1)}.
  //
  // Requires min + (n-1) to not overflow T.
  //
  // Returns status on failure. See `DistinctIntegers()` for simpler API
  // version.
  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> TryDistinctIntegers(T n, int k, T min = 0);

  // RandomComposition()
  //
  // Returns a random composition (a partition where the order of the buckets is
  // important) with k buckets. Each bucket has at least `min_bucket_size`
  // elements and the sum of the `k` bucket sizes is `n`.
  //
  // The returned values are the sizes of each bucket. Note that (1, 2) is
  // different from (2, 1).
  //
  // Requires n + (k - 1) to not overflow T.
  //
  // Crashes on failure. See `TryRandomComposition()` for a non-crashing
  // version.
  template <typename T>
    requires std::integral<T>
  std::vector<T> RandomComposition(T n, int k, T min_bucket_size = 1);

  // TryRandomComposition()
  //
  // Returns a random composition (a partition where the order of the buckets is
  // important) with k buckets. Each bucket has at least `min_bucket_size`
  // elements and the sum of the `k` bucket sizes is `n`.
  //
  // The returned values are the sizes of each bucket. Note that (1, 2) is
  // different from (2, 1).
  //
  // Requires n + (k - 1) to not overflow T.
  //
  // Returns status on failure. See `RandomComposition()` for simpler API
  // version.
  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> TryRandomComposition(T n, int k,
                                                      T min_bucket_size = 1);

  // GetVariable()
  //
  // Returns a copy of the variable `variable_name` from global context. Note
  // that changes you make to this variable will not impact the underlying
  // variable. Use `SetVariable()` or `ConstrainVariable()` to do that.
  //
  // Crashes on failure. See `TryGetVariable()` for a non-crashing version.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  [[nodiscard]] T GetVariable(absl::string_view variable_name) const;

  // TryGetVariable()
  //
  // Returns a copy of the variable `variable_name` from global context. Note
  // that changes you make to this variable will not impact the underlying
  // variable. Use `SetVariable()` or `ConstrainVariable()` to do that.
  //
  // Returns status on failure. See `GetVariable()` for simpler API version.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<T> TryGetVariable(absl::string_view variable_name) const;

  // SizedValue()
  //
  // Returns a value generated by `variable_name` of size `size`.
  //
  // Prefer `MinValue()`, `MaxValue()`, `SmallValue()`, etc. for common sizes.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> SizedValue(
      absl::string_view variable_name, absl::string_view size);

  // MinValue()
  // TinyValue()
  // SmallValue()
  // MediumValue()
  // LargeValue()
  // HugeValue()
  // MaxValue()
  //
  // Returns a value generated by `variable_name` of the appropriate size.
  //
  // `T` is the desired output type. Example:
  //   int x = MinValue<int>("x");
  //
  // Crashes on failure. See `TryXXXValue()` for non-crashing versions.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  [[nodiscard]] T::value_type MinValue(absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  [[nodiscard]] T::value_type TinyValue(absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  [[nodiscard]] T::value_type SmallValue(absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  [[nodiscard]] T::value_type MediumValue(absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  [[nodiscard]] T::value_type LargeValue(absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  [[nodiscard]] T::value_type HugeValue(absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  [[nodiscard]] T::value_type MaxValue(absl::string_view variable_name);

  // TryMinValue()
  // TryTinyValue()
  // TrySmallValue()
  // TryMediumValue()
  // TryLargeValue()
  // TryHugeValue()
  // TryMaxValue()
  //
  // Returns a value generated by `variable_name` of the appropriate size.
  //
  // `T` is the desired output type. Example:
  //   absl::StatusOr<int> x = TryMinValue<int>("x");
  //
  // Returns status on failure. See `XXXValue()` for simpler API versions.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> TryMinValue(
      absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> TryTinyValue(
      absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> TrySmallValue(
      absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> TryMediumValue(
      absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> TryLargeValue(
      absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> TryHugeValue(
      absl::string_view variable_name);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> TryMaxValue(
      absl::string_view variable_name);

  // SatisfiesConstraints()
  //
  // Checks if `value` satisfies the constraints in `constraints`. Returns a
  // non-ok status with the reason if not. You may use other known variables in
  // your constraints. Examples:
  //
  //   absl::Status s = SatisfiesConstraints(MInteger().AtMost("N"), 25);
  //   absl::Status s = SatisfiesConstraints(MString().OfLength(5), "hello");
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::Status SatisfiesConstraints(T constraints,
                                    const T::value_type& value) const;

 private:
  // Pointers are to maintain reference-stability with AddCase()
  // TODO(darcybest): This should be unique_ptr, but having it as shared_ptr so
  // we can copy freely for now.
  std::vector<std::shared_ptr<TestCase>> test_cases_;

  // List of optional test cases.
  std::vector<std::shared_ptr<TestCase>> optional_test_cases_;

  // `rng_` is not initialized until InternalSetSeed is called.
  std::optional<moriarty_internal::RandomEngine> rng_;

  // `general_constraints_` are the constraints of all variables declared in the
  // Moriarty class.
  std::optional<const moriarty_internal::VariableSet> general_constraints_;

  // `scenarios_` are passed to all future calls to `AddTestCase()`.
  std::vector<Scenario> scenarios_;

  // TODO(darcybest): This should be part of GenerationConfig.
  // Approximately how many tokens to generate.
  std::optional<int64_t> approximate_generation_limit_;

  // ---------------------------------------------------------------------------
  //    Start of Internal Extended API
  //
  // These functions can be accessed via `moriarty_internal::ImporterManager`.
  // Users and Librarians should not need to access these functions. See
  // `ImporterManager` for more details.
  friend class moriarty_internal::GeneratorManager;

  // SetSeed()
  //
  // Sets the seed for the random engine used inside the generator.
  void SetSeed(absl::Span<const int64_t> seed);

  // SetGeneralConstraints()
  //
  // Sets the general constraints for test case generation. Each TestCase
  // generated by the generator must consider these constraints as well as the
  // ones given by the user.
  void SetGeneralConstraints(
      moriarty_internal::VariableSet general_constraints);

  // SetApproximateGenerationLimit()
  //
  // Sets a threshold for approximately how much data to generate. If set,
  // Moriarty will call generators until is has generated `limit` "units", where
  // a "unit" is computed as: 1 for integers, size for string, and sum of number
  // of units of its elements for arrays. All other MVariables will have a size
  // of 1.
  //
  // None of these values are guaranteed to remain the same in the future and
  // this function is a suggestion to Moriarty, not a guarantee that it will
  // stop generation at any point.
  void SetApproximateGenerationLimit(int64_t limit);

  // GetTestCases()
  //
  // Returns the internal list of test cases.
  const std::vector<std::shared_ptr<TestCase>>& GetTestCases();

  // GetGeneralConstraints()
  //
  // Returns the internal list of general constraints.
  std::optional<const moriarty_internal::VariableSet> GetGeneralConstraints();

  // GetOptionalTestCases()
  //
  // Returns the internal list of test cases.
  const std::vector<std::shared_ptr<TestCase>>& GetOptionalTestCases();

  // AssignValuesInAllTestCases()
  //
  // Assigns all of the variables in all of the test cases.
  absl::StatusOr<std::vector<moriarty_internal::ValueSet>>
  AssignValuesInAllTestCases();

  // ClearCases()
  //
  // Removes all cases that have been generated.
  void ClearCases();

  // GetRandomEngine()
  //
  // Returns a reference to the internal random engine, or nullptr if it is
  // unavailable.
  absl::Nullable<moriarty_internal::RandomEngine*> GetRandomEngine();

  //    End of Internal Extended API
  // ---------------------------------------------------------------------------
};

namespace moriarty_internal {

// GeneratorManager [Internal Extended API]
//
// Contains functions that are public with respect to `Generator`, but should be
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
// See corresponding functions in `Generator` for documentation.
//
// Example usage:
//   class ExampleGenerator : public moriarty::Generator { ... };
//
//   ExampleGenerator g;
//   moriarty::moriarty_internal::GeneratorManager(&g).SetSeed( ... );
class GeneratorManager {
 public:
  // This class does not take ownership of `generator_to_manage`
  explicit GeneratorManager(moriarty::Generator* generator_to_manage);

  void SetSeed(absl::Span<const int64_t> seed);
  void SetGeneralConstraints(VariableSet general_constraints);
  void SetApproximateGenerationLimit(int64_t limit);
  const std::vector<std::shared_ptr<TestCase>>& GetTestCases();
  std::optional<const moriarty_internal::VariableSet> GetGeneralConstraints();
  const std::vector<std::shared_ptr<TestCase>>& GetOptionalTestCases();
  absl::StatusOr<std::vector<ValueSet>> AssignValuesInAllTestCases();
  void ClearCases();
  absl::Nullable<moriarty_internal::RandomEngine*> GetRandomEngine();

 private:
  Generator& managed_generator_;  // Not owned by this class
};

}  // namespace moriarty_internal

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
T::value_type Generator::Random(T m) {
  return moriarty_internal::TryFunctionOrCrash<typename T::value_type>(
      [&, this]() { return this->TryRandom<T>(std::move(m)); }, "Random");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Generator::TryRandom(T m) {
  if (rng_ == std::nullopt) {
    return MisconfiguredError("Generator", "TryRandom",
                              InternalConfigurationType::kRandomEngine);
  }

  // We make a copy of the variables so that they may be generated if needed by
  // the user without affecting the global scope.
  moriarty_internal::VariableSet variables =
      general_constraints_.value_or(moriarty_internal::VariableSet());
  moriarty_internal::ValueSet values;
  moriarty_internal::GenerationConfig generation_config;

  moriarty_internal::Universe universe =
      moriarty_internal::Universe()
          .SetRandomEngine(&(*rng_))
          .SetMutableVariableSet(&variables)
          .SetMutableValueSet(&values)
          .SetGenerationConfig(&generation_config);

  moriarty_internal::MVariableManager(&m).SetUniverse(
      &universe,
      /* my_name_in_universe = */ absl::Substitute("TryRandom($0)",
                                                   m.Typename()));

  return moriarty_internal::MVariableManager(&m).Generate();
}

template <typename T>
absl::Status Generator::TryShuffle(std::vector<T>& container) {
  if (!rng_) {
    return MisconfiguredError("Generator", "Shuffle",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::Shuffle(*rng_, container);
}

template <typename T>
absl::StatusOr<T> Generator::TryRandomElement(const std::vector<T>& container) {
  if (!rng_) {
    return MisconfiguredError("Generator", "RandomElement",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomElement(*rng_, container);
}

template <typename T>
absl::StatusOr<std::vector<T>> Generator::TryRandomElementsWithReplacement(
    const std::vector<T>& container, int k) {
  if (!rng_) {
    return MisconfiguredError("Generator", "RandomElementsWithReplacement",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomElementsWithReplacement(*rng_, container, k);
}

template <typename T>
absl::StatusOr<std::vector<T>> Generator::TryRandomElementsWithoutReplacement(
    const std::vector<T>& container, int k) {
  if (!rng_) {
    return MisconfiguredError("Generator", "RandomElementsWithoutReplacement",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomElementsWithoutReplacement(*rng_, container,
                                                             k);
}

template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> Generator::TryRandomPermutation(int n, T min) {
  if (!rng_) {
    return MisconfiguredError("Generator", "RandomPermutation",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomPermutation(*rng_, n, min);
}

template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> Generator::TryDistinctIntegers(T n, int k,
                                                              T min) {
  if (!rng_) {
    return MisconfiguredError("Generator", "DistinctIntegers",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::DistinctIntegers(*rng_, n, k, min);
}

template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> Generator::TryRandomComposition(
    T n, int k, T min_bucket_size) {
  if (!rng_) {
    return MisconfiguredError("Generator", "RandomComposition",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomComposition(*rng_, n, k, min_bucket_size);
}

template <typename T>
void Generator::Shuffle(std::vector<T>& container) {
  moriarty_internal::TryFunctionOrCrash(
      [&]() { return this->TryShuffle(container); }, "Shuffle");
}

template <typename T>
T Generator::RandomElement(const std::vector<T>& container) {
  return moriarty_internal::TryFunctionOrCrash<T>(
      [&]() { return this->TryRandomElement(container); }, "RandomElement");
}

template <typename T>
std::vector<T> Generator::RandomElementsWithReplacement(
    const std::vector<T>& container, int k) {
  return moriarty_internal::TryFunctionOrCrash<std::vector<T>>(
      [&]() { return this->TryRandomElementsWithReplacement(container, k); },
      "RandomElementWithReplacement");
}

template <typename T>
std::vector<T> Generator::RandomElementsWithoutReplacement(
    const std::vector<T>& container, int k) {
  return moriarty_internal::TryFunctionOrCrash<std::vector<T>>(
      [&]() { return this->TryRandomElementsWithoutReplacement(container, k); },
      "RandomElementWithoutReplacement");
}

template <typename T>
  requires std::integral<T>
std::vector<T> Generator::RandomPermutation(int n, T min) {
  return moriarty_internal::TryFunctionOrCrash<std::vector<T>>(
      [&]() { return this->TryRandomPermutation(n, min); },
      "RandomPermutation");
}

template <typename T>
  requires std::integral<T>
std::vector<T> Generator::DistinctIntegers(T n, int k, T min) {
  return moriarty_internal::TryFunctionOrCrash<std::vector<T>>(
      [&]() { return this->TryDistinctIntegers(n, k, min); },
      "DistinctIntegers");
}

template <typename T>
  requires std::integral<T>
std::vector<T> Generator::RandomComposition(T n, int k, T min_bucket_size) {
  return moriarty_internal::TryFunctionOrCrash<std::vector<T>>(
      [&]() { return this->TryRandomComposition(n, k, min_bucket_size); },
      "RandomComposition");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
T Generator::GetVariable(absl::string_view variable_name) const {
  return moriarty_internal::TryFunctionOrCrash<typename T::value_type>(
      [&, this]() { return this->TryGetVariable<T>(variable_name); },
      "GetVariable");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<T> Generator::TryGetVariable(
    absl::string_view variable_name) const {
  if (!general_constraints_) {
    return MisconfiguredError("Generator", "TryGetVariable",
                              InternalConfigurationType::kVariableSet);
  }

  return general_constraints_->GetVariable<T>(variable_name);
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Generator::SizedValue(
    absl::string_view variable_name, absl::string_view size) {
  MORIARTY_ASSIGN_OR_RETURN(auto variable, TryGetVariable<T>(variable_name));
  MORIARTY_RETURN_IF_ERROR(variable.TryWithKnownProperty(
      Property({.category = "size", .descriptor = std::string(size)})));
  return TryRandom(variable);
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
typename T::value_type Generator::MinValue(absl::string_view variable_name) {
  return moriarty_internal::TryFunctionOrCrash<typename T::value_type>(
      [&, this]() { return this->TryMinValue<T>(variable_name); }, "MinValue");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
typename T::value_type Generator::TinyValue(absl::string_view variable_name) {
  return moriarty_internal::TryFunctionOrCrash<typename T::value_type>(
      [&, this]() { return this->TryTinyValue<T>(variable_name); },
      "TinyValue");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
typename T::value_type Generator::SmallValue(absl::string_view variable_name) {
  return moriarty_internal::TryFunctionOrCrash<typename T::value_type>(
      [&, this]() { return this->TrySmallValue<T>(variable_name); },
      "SmallValue");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
typename T::value_type Generator::MediumValue(absl::string_view variable_name) {
  return moriarty_internal::TryFunctionOrCrash<typename T::value_type>(
      [&, this]() { return this->TryMediumValue<T>(variable_name); },
      "MediumValue");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
typename T::value_type Generator::LargeValue(absl::string_view variable_name) {
  return moriarty_internal::TryFunctionOrCrash<typename T::value_type>(
      [&, this]() { return this->TryLargeValue<T>(variable_name); },
      "LargeValue");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
typename T::value_type Generator::HugeValue(absl::string_view variable_name) {
  return moriarty_internal::TryFunctionOrCrash<typename T::value_type>(
      [&, this]() { return this->TryHugeValue<T>(variable_name); },
      "HugeValue");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
typename T::value_type Generator::MaxValue(absl::string_view variable_name) {
  return moriarty_internal::TryFunctionOrCrash<typename T::value_type>(
      [&, this]() { return this->TryMaxValue<T>(variable_name); }, "MaxValue");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Generator::TryMinValue(
    absl::string_view variable_name) {
  return SizedValue<T>(variable_name, "min");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Generator::TryTinyValue(
    absl::string_view variable_name) {
  return SizedValue<T>(variable_name, "tiny");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Generator::TrySmallValue(
    absl::string_view variable_name) {
  return SizedValue<T>(variable_name, "small");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Generator::TryMediumValue(
    absl::string_view variable_name) {
  return SizedValue<T>(variable_name, "medium");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Generator::TryLargeValue(
    absl::string_view variable_name) {
  return SizedValue<T>(variable_name, "large");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Generator::TryHugeValue(
    absl::string_view variable_name) {
  return SizedValue<T>(variable_name, "huge");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Generator::TryMaxValue(
    absl::string_view variable_name) {
  return SizedValue<T>(variable_name, "max");
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::Status Generator::SatisfiesConstraints(T constraints,
                                             const T::value_type& value) const {
  if (!general_constraints_) {
    return MisconfiguredError("Generator", "SatisfiesConstraints",
                              InternalConfigurationType::kVariableSet);
  }

  return moriarty_internal::SatisfiesConstraints(
      std::move(constraints), value,
      /*known_values = */ {}, *general_constraints_,
      "Generator::SatisfiesConstraints");
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_GENERATOR_H_
