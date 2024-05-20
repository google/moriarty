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

#ifndef MORIARTY_SRC_LIBRARIAN_MVARIABLE_H_
#define MORIARTY_SRC_LIBRARIAN_MVARIABLE_H_

#include <algorithm>
#include <any>
#include <concepts>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/constraint_values.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/generation_config.h"
#include "src/internal/random_config.h"
#include "src/internal/random_engine.h"
#include "src/internal/status_utils.h"
#include "src/internal/universe.h"
#include "src/internal/variable_name_utils.h"
#include "src/librarian/io_config.h"
#include "src/librarian/subvalues.h"
#include "src/property.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

namespace moriarty_internal {
template <typename V, typename G>
class MVariableManager;  // Forward declaring the internal API
}  // namespace moriarty_internal

namespace librarian {

// MVariable<>
//
// All Moriarty variable types (e.g., MInteger, MString, MGraph, etc) should
// derive from this class using CRTP (curiously recurring template pattern).
//
// * `VariableType` is the underlying Moriarty variable type
// * `ValueType` is the non-Moriarty type that is generated/validated/analyzed
//   by this MVariable.
//
// E.g.,
// class MInteger : public moriarty::librarian::MVariable<MInteger, int64_t> {}
template <typename VariableType, typename ValueType>
class MVariable : public moriarty_internal::AbstractVariable {
 public:
  using variable_type = VariableType;
  using value_type = ValueType;

  ~MVariable() override = default;

 protected:
  // Only derived classes should make copies of me directly to avoid accidental
  // slicing. Call derived class's constructors instead.
  MVariable() {
    static_assert(std::default_initializable<VariableType>,
                  "Moriarty needs to be able to construct default versions of "
                  "your MVariable using its default constructor.");
    static_assert(std::copyable<VariableType>,
                  "Moriarty needs to be able to copy your MVariable.");
    static_assert(
        std::derived_from<
            VariableType,
            MVariable<VariableType, typename VariableType::value_type>>,
        "The first template argument of MVariable<> must be "
        "VariableType. E.g., class MFoo : public MVariable<MFoo, Foo> {}");
  }
  MVariable(const MVariable&) = default;
  MVariable(MVariable&&) = default;
  MVariable& operator=(const MVariable&) = default;
  MVariable& operator=(MVariable&&) = default;

 public:
  // Typename() [pure virtual]
  //
  // Returns a string representing the name of this type (for example,
  // "MInteger"). This is mostly used for debugging/error messages.
  [[nodiscard]] std::string Typename() const override = 0;

  // ToString()
  //
  // Returns a string representation of the constraints this MVariable has.
  [[nodiscard]] std::string ToString() const;

  // Is()
  //
  // Declares that this variable must be exactly `value`. For example,
  //
  //   M.AddVariable("N", MInteger().Is(5));
  //
  // states that this variable must be 5. Logically equivalent to
  // `IsOneOf({value});`
  VariableType& Is(ValueType value);

  // IsOneOf()
  //
  // Declares that this variable must be exactly one of the elements in
  // `values`. For example,
  //
  //   M.AddVariable("N", MInteger().IsOneOf({5, 10, 15}));
  //   M.AddVariable("S", MString().IsOneOf({"POSSIBLE", "IMPOSSIBLE"});
  //
  // states that the variable "N" must be either 5, 10, or 15, and that the
  // variable "S" must be either the string "POSSIBLE" or "IMPOSSIBLE".
  VariableType& IsOneOf(std::vector<ValueType> values);

  // MergeFrom()
  //
  // Merges my current constraints with the constraints of the `other` variable.
  // The merge should act as an intersection of the two constraints. If one says
  // 1 <= x <= 10 and the other says 5 <= x <= 20, then then merged version
  // should have 5 <= x <= 10.
  //
  // Crashes on failure. See `TryMergeFrom()` for non-crashing version.
  VariableType& MergeFrom(const VariableType& other);

  // TryMergeFrom()
  //
  // Merges my current constraints with the constraints of the `other` variable.
  // The merge should act as an intersection of the two constraints. If one says
  // 1 <= x <= 10 and the other says 5 <= x <= 20, then then merged version
  // should have 5 <= x <= 10.
  //
  // Returns status on failure. See `MergeFrom()` for builder-like version.
  absl::Status TryMergeFrom(const VariableType& other);

  // AddCustomConstraint()
  //
  // Adds a constraint to this variable. `checker` must take exactly one
  // parameter of type `ValueType`. `checker` should return true if the value
  // satisfies this constraint.
  VariableType& AddCustomConstraint(
      absl::string_view constraint_name,
      std::function<bool(const ValueType&)> checker);

  // AddCustomConstraint()
  //
  // Adds a constraint to this variable. `checker` must take exactly two
  // parameters of type `ValueType` and `ConstraintValues`. `checker` should
  // return true if the value satisfies this constraint.
  //
  // `deps` is a list of variables that this constraint depends on. If a
  // variable's name is in this list, you may use ConstraintValues::GetValue<>()
  // to access that variable's value.
  VariableType& AddCustomConstraint(
      absl::string_view constraint_name, std::vector<std::string> deps,
      std::function<bool(const ValueType&, const ConstraintValues& variables)>
          checker);

  // WithKnownProperty()
  //
  // Adds a known property to this variable.
  //
  // Crashes on failure. See `TryWithKnownProperty()` for non-crashing version.
  VariableType& WithKnownProperty(Property property);

  // TryWithKnownProperty()
  //
  // Adds a known property to this variable.
  //
  // Returns status on failure. See `WithKnownProperty()` for simpler API
  // version.
  absl::Status TryWithKnownProperty(Property property);

  // GetDifficultCases()
  //
  // Returns a list of MVariables that where merged with your variable. This
  // should give common difficult cases. Some examples of this type of cases
  // include common pitfalls for your data type (edge cases, queries of death,
  // etc).
  [[nodiscard]] absl::StatusOr<std::vector<VariableType>>
  GetDifficultInstances() const;

 protected:
  // ---------------------------------------------------------------------------
  //  Functions that need to be overridden by all `MVariable`s.

  // GenerateImpl() [virtual]
  //
  // Users should not call this directly. Call `Random(MVariable)` in the
  // specific Moriarty component (Generator, Importer, etc).
  //
  // Returns a random value based on all current constraints on this MVariable.
  //
  // This function may be called several times and should not update the
  // variable's internal state.
  //
  // TODO(darcybest): Consider if this function could/should be `const`.
  // Librarians are strongly encouraged to treat this function as if it were
  // const so you are future compatible.
  //
  // GenerateImpl() will only be called if Is()/IsOneOf() have not been called.
  virtual absl::StatusOr<ValueType> GenerateImpl() = 0;

  // IsSatisfiedWithImpl() [virtual]
  //
  // Users should not call this directly. Call `SatisfiesConstraints()` instead.
  //
  // Determines if `value` satisfies all of the constraints specified by this
  // variable.
  //
  // This function should return an `UnsatisfiedConstraintError()`. Almost every
  // other status returned will be treated as an internal error. The exceptions
  // are `VariableNotFoundError` and `ValueNotFoundError`, which will be
  // converted into an `UnsatisfiedConstraintError` before returning to the
  // user. You may find the `CheckConstraint()` helpers useful.
  virtual absl::Status IsSatisfiedWithImpl(const ValueType& value) const = 0;

  // MergeFromImpl() [virtual]
  //
  // Users should not call this directly. Call `MergeFrom()` instead.
  //
  // Merges my current constraints with the constraints of the `other` variable.
  // The merge should act as an intersection of the two constraints. If one says
  // 1 <= x <= 10 and the other says 5 <= x <= 20, then then merged version
  // should have 5 <= x <= 10.
  virtual absl::Status MergeFromImpl(const VariableType& other) = 0;

  // ReadImpl() [virtual/optional]
  //
  // Users should not call this directly. Call `Read()` instead.
  //
  // Reads a value from the input stream provided by the IOConfig.
  //
  // * Return a non-OK status if parsing fails.
  virtual absl::StatusOr<ValueType> ReadImpl();

  // PrintImpl() [virtual/optional]
  //
  // Users should not call this directly. Call `Print()` instead.
  //
  // Print `value` into `os` using any configuration provided by your
  // VariableType (whitespace separators, etc).
  virtual absl::Status PrintImpl(const ValueType& value);

  // GetDependenciesImpl() [virtual/optional]
  //
  // Users should not call this directly. Call `GetDependencies()` instead.
  //
  // Returns a list of the variable names that this variable depends on.
  //
  // Default: No dependencies.
  virtual std::vector<std::string> GetDependenciesImpl() const;

  // GetSubvaluesImpl() [virtual/optional]
  //
  // Users should not call this directly. Call `GetSubvalue()` instead.
  //
  // Returns all subvalues of `value`. Subvalues are non-Moriarty types (e.g.,
  // int64_t, std::string) not MVariables (e.g., MInteger, MString).
  //
  // Example implementation:
  //
  //  absl::StatusOr<Subvalues> CustomType::GetSubvalues() {
  //     return Subvalues()
  //         .AddSubvalue("length", array.length())
  //         .AddSubvariable("width", array[0].length());
  //  }
  //
  // Default: No subvalues.
  virtual absl::StatusOr<Subvalues> GetSubvaluesImpl(
      const ValueType& value) const;

  // GetDifficultInstancesImpl() [virtual/optional]
  //
  // Users should not call this directly. Call `GetDifficultInstances()`
  // instead.
  //
  // Returns a list of complicated instances to merge from.
  // TODO(hivini): Improve comment with examples.
  virtual absl::StatusOr<std::vector<VariableType>> GetDifficultInstancesImpl()
      const;

  // GetUniqueValueImpl() [virtual/optional]
  //
  // Users should not call this directly. Call `MergeFrom()` instead.
  //
  // Determines if there is exactly one value that this variable can be assigned
  // to. If so, return that value. If there is not a unique value (or it is too
  // hard to determine that there is a unique value), returns `std::nullopt`.
  // Returning `std::nullopt` does not guarantee there is not a unique value,
  // just that it is too hard to determine it.
  //
  // By default, this returns `std::nullopt`.
  virtual std::optional<ValueType> GetUniqueValueImpl() const;

  // ToStringImpl() [virtual/optional]
  //
  // Returns the constraints on this variable in a string format so the user can
  // use it for a better debugging experience.
  //
  // MVariable will handle `Is()` and `IsOneOf()`, this should just handle extra
  // information specific to your type.
  virtual std::string ToStringImpl() const;

  // ValueToStringImpl() [virtual/optional]
  //
  // Returns `value` in string format. This is used for error messages to the
  // user.
  virtual absl::StatusOr<std::string> ValueToStringImpl(
      const ValueType& value) const;

  // ---------------------------------------------------------------------------
  //  Functions to fully register this MVariable with Moriarty.

  // PropertyCallbackFunction is a *pointer-to-member-function* to some method
  // of the underlying variable type (e.g., MInteger, MString).
  using PropertyCallbackFunction = absl::Status (VariableType::*)(Property);

  // RegisterKnownProperty() [Registration Point for Librarians]
  //
  // Informs MVariable that `property_category` is a property that the derived
  // class knows how to interpret. When this category is requested, MVariable
  // will call `property_fn`. This should return `absl::OkStatus` if the
  // property was interpreted and handled properly.
  //
  // This function *must* be a member function of your class and not a
  // free-function.
  //
  // Example call:
  //   RegisterKnownProperty("size", &MInteger::WithSizeProperty);
  //
  // With the corresponding function:
  //   absl::Status MInteger::WithSizeProperty(Property property) { ... }
  void RegisterKnownProperty(absl::string_view property_category,
                             PropertyCallbackFunction property_fn);

  // ---------------------------------------------------------------------------
  //  Functions for librarian use only

  // Random() [Helper for Librarians]
  //
  // Generates a random value that is described by `m`.
  //
  // `debug_name` is for better debugging messages on failure and is local
  // only to this function call.
  //
  // Example usage:
  //    absl::StatusOr<int64_t> x = Random("my_int",
  //                                       MInteger().Between(1, 10));
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> Random(absl::string_view debug_name,
                                                T m);

  // SatisfiesConstraints() [Helper for Librarians]
  //
  // Determines if `value` satisfies the constraints of `m`. Any global context
  // needed will come from *this* variable, not `m`.
  //
  // For example, if `m = MInteger().Between(1, "N")`, then the value of "N" is
  // from *this* variable's context, not `m`'s context.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::Status SatisfiesConstraints(T m, const T::value_type& value) const;

  // Read() [Helper for Librarians]
  //
  // Reads a value using any configuration provided by `m` (whitespace
  // separators, etc) using *this* variable's underlying istream. The underlying
  // istream is set by a Moriarty component (Generator, Exporter, Moriarty, etc)
  // via its Universe.
  //
  // If the read is invalid, the underlying istream is now in an unspecified
  // state.
  //
  // `debug_name` is for better debugging messages on failure and is local
  // only to this function call.
  //
  // Example usage:
  //    absl::StatusOr<int64_t> x = Read("my_int", MInteger().Between(1, 10));
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> Read(absl::string_view debug_name,
                                              T m);

  // Print() [Helper for Librarians]
  //
  // Print `value` into *this* variable's underlying ostream using any
  // configuration (whitespace separators, etc) provided by `m`.
  //
  // The underlying ostream is set by a Moriarty component (Generator, Exporter,
  // Moriarty, etc) via its Universe.
  //
  // `debug_name` is for better debugging messages on failure and is local
  // only to this function call.
  //
  // Example usage:
  //    absl::Status success = Print("my_int", MInteger(), 5);
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::Status Print(absl::string_view debug_name, T m,
                     const T::value_type& value);

  // GetKnownValue() [Helper for Librarians]
  //
  // Returns the value of a known variable. If the variable is not known,
  // returns `ValueNotFoundError`.
  //
  // Example Usage:
  //  int64_t N = GetKnownValue<MInteger>("N");
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> GetKnownValue(
      absl::string_view variable_name) const;

  // GenerateValue() [Helper for Librarians]
  //
  // Returns the value of a variable. If the variable's value is not known, it
  // will be generated now.
  //
  // Example Usage:
  //  int64_t N = GenerateValue<MInteger>("N");
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::StatusOr<typename T::value_type> GenerateValue(
      absl::string_view variable_name);

  // GetUniqueValue() [Helper for Librarians]
  //
  // Determines if there is exactly one value that `m` can be assigned to. If
  // so, return that value. If there is not a unique value (or it is too hard to
  // determine that there is a unique value), returns `std::nullopt`.
  // Returning `std::nullopt` does not guarantee there is not a unique value,
  // just that it is too hard to determine it.
  //
  // `debug_name` is for better debugging messages on failure and is local
  // only to this function call.
  //
  // Example Usage:
  //  std::optional<int64_t> N = GetUniqueValue("N", MInteger().Between(7, 7));
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  std::optional<typename T::value_type> GetUniqueValue(
      absl::string_view debug_name, T m) const;

  // RandomInteger() [Helper for Librarians]
  //
  // Returns a random integer in the closed interval [min, max].
  absl::StatusOr<int64_t> RandomInteger(int64_t min, int64_t max);

  // RandomInteger() [Helper for Librarians]
  //
  // Returns a random integer in the semi-closed interval [0, n). Useful for
  // random indices.
  absl::StatusOr<int64_t> RandomInteger(int64_t n);

  // Shuffle() [Helper for Librarians]
  //
  // Shuffles the elements in `container`.
  template <typename T>
  absl::Status Shuffle(std::vector<T>& container);

  // RandomElement() [Helper for Librarians]
  //
  // Returns a random element of `container`.
  template <typename T>
  absl::StatusOr<T> RandomElement(const std::vector<T>& container);

  // RandomElementsWithReplacement() [Helper for Librarians]
  //
  // Returns k (randomly ordered) elements of `container`, possibly with
  // duplicates.
  template <typename T>
  absl::StatusOr<std::vector<T>> RandomElementsWithReplacement(
      const std::vector<T>& container, int k);

  // RandomElementsWithoutReplacement() [Helper for Librarians]
  //
  // Returns k (randomly ordered) elements of `container`, without duplicates.
  //
  // Each element may appear at most once. Note that if there are duplicates
  // in `container`, each of those could be returned once each.
  //
  // So RandomElementsWithoutReplacement({0, 1, 1, 1}, 2) could return {1, 1}.
  template <typename T>
  absl::StatusOr<std::vector<T>> RandomElementsWithoutReplacement(
      const std::vector<T>& container, int k);

  // RandomPermutation() [Helper for Librarians]
  //
  // Returns a random permutation of {0, 1, ... , n-1}.
  absl::StatusOr<std::vector<int>> RandomPermutation(int n);

  // RandomPermutation() [Helper for Librarians]
  //
  // Returns a random permutation of {min, min + 1, ... , min + (n-1)}.
  //
  // Requires min + (n-1) to not overflow T.
  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> RandomPermutation(int n, T min);

  // DistinctIntegers() [Helper for Librarians]
  //
  // Returns k (randomly ordered) distinct integers from
  // {min, min + 1, ... , min + (n-1)}.
  //
  // Requires min + (n-1) to not overflow T.
  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> DistinctIntegers(T n, int k, T min = 0);

  // RandomComposition() [Helper for Librarians]
  //
  // Returns a random composition (a partition where the order of the buckets
  // is important) with k buckets. Each bucket has at least `min_bucket_size`
  // elements and the sum of the `k` bucket sizes is `n`.
  //
  // The returned values are the sizes of each bucket. Note that (1, 2) is
  // different from (2, 1).
  //
  // Requires n + (k - 1) to not overflow T.
  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> RandomComposition(T n, int k,
                                                   T min_bucket_size = 1);

  // GetIOConfig() [Helper for Librarians]
  //
  // Returns the IOConfig. All reading/writing in Moriarty should be aided by
  // this IOConfig.
  absl::StatusOr<absl::Nonnull<const IOConfig*>> GetIOConfig() const;

  // GetIOConfig() [Helper for Librarians]
  //
  // Returns the IOConfig. All reading/writing in Moriarty should be aided by
  // this IOConfig.
  absl::StatusOr<absl::Nonnull<IOConfig*>> GetIOConfig();

  // GetApproximateGenerationLimit() [Helper for Librarians]
  //
  // Returns the threshold for approximately how much data to generate. If
  // set, Moriarty will call generators until is has generated `limit`
  // "units", where a "unit" is computed as: 1 for integers, size for string,
  // and sum of number of units of its elements for arrays. All other
  // MVariables will have a size of 1.
  //
  // Is the returned value is std::nullopt, then there is no limit set.
  //
  // None of these values are guaranteed to remain the same in the future and
  // this function is a suggestion to Moriarty, not a guarantee that it will
  // stop generation at any point.
  [[nodiscard]] std::optional<int64_t> GetApproximateGenerationLimit() const;

  // GetDependencies() [Helper for Librarians]
  //
  // Returns the list of names of the variables that `variable` depends on,
  // this includes direct dependencies and the recursive child dependencies.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  std::vector<std::string> GetDependencies(T variable) const;

 private:
  // `is_one_of_` is a list of values that Generate() should produce. If the
  // optional is set and the list is empty, then there are no viable values.
  std::optional<std::vector<ValueType>> is_one_of_;

  // `universe_` is set iff SetUniverse has been called.
  // The universe contains all context of the outside world around this
  // variable (for example, which random generator to use and other variables
  // it may request in the same VariableSet).
  moriarty_internal::Universe* universe_ = nullptr;

  // `variable_name_inside_universe_` is the name to look up in
  // `universe_` when information about this variable is needed. The
  // variable stored in `universe_` need not be exactly this variable,
  // but an equivalent representation.
  std::string variable_name_inside_universe_;

  // `custom_constraints_deps_` is a list of the needed variables for all the
  // custom constraints defined for this variable.
  std::vector<std::string> custom_constraints_deps_;

  // `custom_constraints_` is a list of constraints, in the form of functions,
  // to check when `SatisfiesConstraints()` is called.
  struct CustomConstraint {
    std::string name;
    std::function<bool(const ValueType&, const ConstraintValues& cv)> checker;
  };
  std::vector<CustomConstraint> custom_constraints_;

  // The known properties of this variable. Maps from category -> function.
  absl::flat_hash_map<std::string, PropertyCallbackFunction>
      known_property_categories_;

  // ---------------------------------------------------------------------------
  //    Start of Internal Extended API
  //
  // These functions can be accessed via `moriarty_internal::ImporterManager`.
  // Users and Librarians should not need to access these functions. See
  // `ImporterManager` for more details.
  friend class moriarty_internal::MVariableManager<VariableType, ValueType>;

  // Generate() [Internal Extended API]
  //
  // Generates a random value for the constraints in this MVariable. This
  // function will essentially first check `Is()` and `IsOneOf()`, then call
  // `GenerateImpl()`.
  //
  // Users should not need to call this function directly. Use
  // `Random(MVariable)` in the appropriate Moriarty component to generate
  // random values.
  absl::StatusOr<ValueType> Generate();

  // IsSatisfiedWith() [Internal Extended API]
  //
  // Determines if `value` satisfies all of the constraints spcecified by this
  // variable, both built-in ones and custom ones added via
  // `AddCustomConstraint()`.
  //
  // * If `value` satisfies all constraints, return `absl::OkStatus()`.
  // * If `value` does not satisfy some constraint, returns an
  //   `UnsatisfiedConstraintError()`.
  // * Otherwise, some other status will be returned and the validity of
  // `value`
  //   is unspecified (this should be rare and is dependent on the specific
  //   MVariable).
  absl::Status IsSatisfiedWith(const ValueType& value) const;

  // MergeFrom() [Internal Extended API]
  //
  // Merges my current constraints with the constraints of the `other`
  // variable. The merge should act as an intersection of the two constraints.
  // If one says 1 <= x <= 10 and the other says 5 <= x <= 20, then then
  // merged version should have 5 <= x <= 10.
  absl::Status MergeFrom(
      const moriarty_internal::AbstractVariable& other) override;

  // TryRead() [Internal Extended API]
  //
  // Reads a value using any configuration provided by your
  // `VariableType` (whitespace separators, etc) using the underlying istream.
  //
  // If the read is invalid, the underlying istream is now in an unspecified
  // state.
  //
  // The underlying istream is set by a Moriarty component (Generator,
  // Exporter, Moriarty, etc) via its Universe.
  // TODO(darcybest): Remove Try.
  absl::StatusOr<ValueType> TryRead();

  // TryPrint() [Internal Extended API]
  //
  // Print `value` into the underlying ostream using any configuration
  // provided by your VariableType (whitespace separators, etc).
  //
  // The underlying ostream is set by a Moriarty component (Generator,
  // Exporter, Moriarty, etc) via its Universe.
  // TODO(darcybest): Remove Try.
  absl::Status TryPrint(const ValueType& value);

  // GetSubvalue() [Internal Extended API]
  //
  // `my_value` will be of the ValueType of this variable.
  //
  // Given the value of this variable and the name of a subvalue, this will
  // return the std::any value of the subvalue if it is found.
  //
  // Returns an error if there are any issues retrieving the subvalue.
  // TODO(darcybest): Consider renaming to ExtractSubvalue.
  absl::StatusOr<std::any> GetSubvalue(
      const std::any& my_value, absl::string_view subvalue_name) const override;

  // GetRandomEngine() [Internal Extended API]
  //
  // Retrieves the internal RandomEngine used for random generation. Only
  // moriarty::MInteger should use this directly for randomization. Everyone
  // else should call Random<int>().
  moriarty_internal::RandomEngine& GetRandomEngine();

  // SetUniverse() [Internal Extended API]
  //
  // Sets the universe for this variable as well as all subvariables.
  void SetUniverse(moriarty_internal::Universe* universe,
                   absl::string_view my_name_in_universe) override;

  //    End of Internal Extended API
  // ---------------------------------------------------------------------------

  // Helper function that casts *this to `VariableType`.
  [[nodiscard]] VariableType& UnderlyingVariableType();
  [[nodiscard]] const VariableType& UnderlyingVariableType() const;

  // Clones the object, similar to a copy-constructor.
  [[nodiscard]] std::unique_ptr<moriarty_internal::AbstractVariable> Clone()
      const override;

  // WithProperty()
  //
  // Tells this variable that it should satisfy `property`.
  absl::Status WithProperty(Property property) override;

  // Try to generate exactly once, without any retries.
  absl::StatusOr<ValueType> GenerateOnce();

  // AssignValue()
  //
  // Given all current constraints, assigns a specific value to this variable
  // via the `Universe`. The value should be stored under the name
  // provided by the SetUniverse method. If the `Universe`
  // already contains a value for this variable, this is a no-op.
  absl::Status AssignValue() override;

  // AssignUniqueValue()
  //
  // Determines if there is exactly one value that this variable can be
  // assigned to. If so, assigns that value.
  //
  // If there is not a unique value (or it is too hard to determine that there
  // is a unique value), this does nothing.
  //
  // Example: MInteger().Between(7, 7) might be able to determine that its
  // unique value is 7.
  absl::Status AssignUniqueValue() override;

  // GetUniqueValueUntyped()
  //
  // Returns the unique value that this variable can be assigned to. If there
  // is not a unique value (or it is too hard to determine that there is one),
  // returns `std::nullopt`. The `std::any` will be a `ValueType`.
  std::optional<std::any> GetUniqueValueUntyped() const override;

  // ValueSatisfiesConstraints()
  //
  // Determines if all variable constraints specified here have a
  // corresponding values (accessed via its Universe) that satisfies the
  // constraints.
  //
  // If a variable does not have a value, this will return not ok.
  // If a value does not have a variable, this will return ok.
  absl::Status ValueSatisfiesConstraints() const override;

  // ReadValue()
  //
  // Given all current I/O constraints on this variable, read a value from
  // `is` and store it in its Universe under the name provided via
  // `SetUniverse`.
  //
  // If reading is not successful, `is` is left in an undefined state.
  absl::Status ReadValue() override;

  // PrintValue()
  //
  // Given all current I/O constraints on this variable, print the value
  // currently stored in Universe under the name provided by
  // SetUniverse into `os`.
  absl::Status PrintValue() override;

  // GetDependencies()
  //
  // Returns the list of names of the variables that this variable depends on,
  // this includes direct dependencies and the recursive child dependencies.
  std::vector<std::string> GetDependencies() override;

  // GetDifficultAbstractVariables()
  //
  // Returns the list of difficult abstract variables defined by the
  // implementation of this abstract variable.
  absl::StatusOr<
      std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>>
  GetDifficultAbstractVariables() const override;
};

}  // namespace librarian

namespace moriarty_internal {

// MVariableManager [Internal Extended API]
//
// Contains functions that are public with respect to `MVariable`, but should be
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
// See corresponding functions in `MVariable` for documentation.
//
// Example usage:
//   class MCustomType :
//     public moriarty::librarian::MVariable<MCustomType, CustomType> { ... };
//
//   MCustomType v;
//   moriarty::moriarty_internal::MVariableManager(&v).GetRandomEngine();
template <typename VariableType, typename ValueType>
class MVariableManager {
 public:
  // This class does not take ownership of `mvariable_to_manage`
  explicit MVariableManager(
      librarian::MVariable<VariableType, ValueType>* mvariable_to_manage);

  absl::StatusOr<ValueType> Generate();
  absl::Status IsSatisfiedWith(const ValueType& value) const;
  absl::Status MergeFrom(const AbstractVariable& other);
  absl::StatusOr<ValueType> TryRead();
  absl::Status TryPrint(const ValueType& value);
  absl::StatusOr<std::any> GetSubvalue(const std::any& my_value,
                                       absl::string_view subvalue_name);
  RandomEngine& GetRandomEngine();
  void SetUniverse(moriarty_internal::Universe* universe,
                   absl::string_view my_name_in_universe);
  std::optional<std::any> GetUniqueValueUntyped() const;
  std::vector<std::string> GetDependencies() const;

 private:
  librarian::MVariable<VariableType, ValueType>& managed_mvariable_;
};

// Class template argument deduction (CTAD). Allows for `MVariableManager(&foo)`
// instead of `MVariableManager<MFoo, Foo>(&foo)`.
template <typename V, typename G>
MVariableManager(librarian::MVariable<V, G>*) -> MVariableManager<V, G>;

}  // namespace moriarty_internal

// -----------------------------------------------------------------------------
//  Template implementation below

// In the template implementations below, we use `MVariable<V, G>` instead of
// `MVariable<VariableType, ValueType>` for readability.
//
// V = VariableType
// G = ValueType  (G since it was previously `GeneratedType`).

namespace librarian {

// -----------------------------------------------------------------------------
//  Template implementation for public functions

template <typename V, typename G>
std::string MVariable<V, G>::ToString() const {
  std::string result = Typename();
  if (is_one_of_) {
    absl::StrAppend(&result,
                    absl::Substitute("; $0 option(s) from Is()/IsOneOf()",
                                     is_one_of_->size()));
    if (!is_one_of_->empty() &&
        !absl::IsUnimplemented(
            ValueToStringImpl(is_one_of_->front()).status())) {
      bool first = true;
      for (const G& value : *is_one_of_) {
        absl::StrAppend(&result, (first ? ": " : ", "),
                        ValueToStringImpl(value).value_or("[ToString error]"));
        first = false;
      }
    }
  }

  return absl::StrCat(result, "; ", ToStringImpl());
}

template <typename V, typename G>
V& MVariable<V, G>::Is(G value) {
  return IsOneOf({std::move(value)});
}

template <typename V, typename G>
V& MVariable<V, G>::IsOneOf(std::vector<G> values) {
  std::sort(values.begin(), values.end());

  if (!is_one_of_) {
    is_one_of_ = std::move(values);
    return UnderlyingVariableType();
  }

  is_one_of_->erase(
      std::set_intersection(std::begin(values), std::end(values),
                            std::begin(*is_one_of_), std::end(*is_one_of_),
                            std::begin(*is_one_of_)),
      std::end(*is_one_of_));
  return UnderlyingVariableType();
}

template <typename V, typename G>
V& MVariable<V, G>::MergeFrom(const V& other) {
  moriarty_internal::TryFunctionOrCrash(
      [this, &other]() { return this->TryMergeFrom(other); }, "MergeFrom");
  return UnderlyingVariableType();
}

template <typename V, typename G>
absl::Status MVariable<V, G>::TryMergeFrom(const V& other) {
  if (other.is_one_of_) IsOneOf(*other.is_one_of_);
  return MergeFromImpl(other);
}

template <typename V, typename G>
V& MVariable<V, G>::AddCustomConstraint(absl::string_view constraint_name,
                                        std::function<bool(const G&)> checker) {
  return AddCustomConstraint(
      constraint_name, {},
      [checker](const G& v, const ConstraintValues& cv) { return checker(v); });
}

template <typename V, typename G>
V& MVariable<V, G>::AddCustomConstraint(
    absl::string_view constraint_name, std::vector<std::string> deps,
    std::function<bool(const G&, const ConstraintValues& cv)> checker) {
  CustomConstraint c;
  c.name = constraint_name;
  c.checker = checker;
  custom_constraints_deps_.insert(custom_constraints_deps_.end(),
                                  std::make_move_iterator(deps.begin()),
                                  std::make_move_iterator(deps.end()));
  // Sort the dependencies so the order of the generation is consistent.
  absl::c_sort(custom_constraints_deps_);
  custom_constraints_.push_back(c);
  return UnderlyingVariableType();
}

template <typename V, typename G>
V& MVariable<V, G>::WithKnownProperty(Property property) {
  moriarty_internal::TryFunctionOrCrash(
      [this, &property]() {
        return this->TryWithKnownProperty(std::move(property));
      },
      "WithKnownProperty");
  return UnderlyingVariableType();
}

template <typename V, typename G>
absl::Status MVariable<V, G>::TryWithKnownProperty(Property property) {
  auto it = known_property_categories_.find(property.category);
  if (it == known_property_categories_.end()) {
    if (property.enforcement == Property::Enforcement::kIgnoreIfUnknown)
      return absl::OkStatus();

    return absl::InvalidArgumentError(
        absl::Substitute("Property with non-optional category '$0' "
                         "requested, but unknown to this variable.",
                         property.category));
  }

  Property::Enforcement enforcement = property.enforcement;  // Grab before move

  // it->second is the callback function provided in RegisterKnownProperty.
  absl::Status status =
      std::invoke(it->second, UnderlyingVariableType(), std::move(property));

  if (!status.ok() && enforcement == Property::Enforcement::kFailIfUnknown) {
    return absl::FailedPreconditionError(absl::Substitute(
        "Failed to add property in WithKnownProperty: $0", status.ToString()));
  }

  return absl::OkStatus();
}

template <typename V, typename G>
absl::StatusOr<std::vector<V>> MVariable<V, G>::GetDifficultInstances() const {
  MORIARTY_ASSIGN_OR_RETURN(std::vector<V> instances,
                            this->GetDifficultInstancesImpl());
  for (V& instance : instances) {
    // TODO(hivini): When merging with a fixed value variable that has the same
    // value a difficult instance, this does not return an error. Determine if
    // this matters or not.
    MORIARTY_RETURN_IF_ERROR(instance.TryMergeFrom(UnderlyingVariableType()));
  }
  return instances;
}

// -----------------------------------------------------------------------------
//  Protected functions' implementations

template <typename V, typename G>
absl::StatusOr<G> MVariable<V, G>::ReadImpl() {
  return absl::UnimplementedError(
      absl::StrCat("Read() not implemented for ", Typename()));
}

template <typename V, typename G>
absl::Status MVariable<V, G>::PrintImpl(const G& value) {
  return absl::UnimplementedError(
      absl::StrCat("Print() not implemented for ", Typename()));
}

template <typename V, typename G>
std::vector<std::string> MVariable<V, G>::GetDependenciesImpl() const {
  return std::vector<std::string>();  // By default, return an empty list.
}

template <typename V, typename G>
absl::StatusOr<Subvalues> MVariable<V, G>::GetSubvaluesImpl(
    const G& value) const {
  return absl::UnimplementedError(
      absl::StrCat("GetSubvalues() not implemented for ", Typename()));
}

template <typename V, typename G>
absl::StatusOr<std::vector<V>> MVariable<V, G>::GetDifficultInstancesImpl()
    const {
  return std::vector<V>();  // By default, return an empty list.
}

template <typename V, typename G>
std::optional<G> MVariable<V, G>::GetUniqueValueImpl() const {
  return std::nullopt;  // By default, return no unique value.
}

template <typename V, typename G>
std::string MVariable<V, G>::ToStringImpl() const {
  return absl::Substitute("[No custom ToString() for $0]", Typename());
}

template <typename V, typename G>
absl::StatusOr<std::string> MVariable<V, G>::ValueToStringImpl(
    const G& value) const {
  return absl::UnimplementedError(
      absl::StrCat("ValueToString() not implemented for ", Typename()));
}

template <typename V, typename G>
void MVariable<V, G>::RegisterKnownProperty(
    absl::string_view property_category, PropertyCallbackFunction property_fn) {
  known_property_categories_[property_category] = property_fn;
}

template <typename V, typename G>
template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> MVariable<V, G>::Random(
    absl::string_view debug_name, T m) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "Random",
                              InternalConfigurationType::kUniverse);
  }

  moriarty_internal::MVariableManager(&m).SetUniverse(
      universe_,
      /* my_name_in_universe = */ moriarty_internal::ConstructVariableName(
          variable_name_inside_universe_, debug_name));
  return moriarty_internal::MVariableManager(&m).Generate();
}

template <typename V, typename G>
template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::Status MVariable<V, G>::SatisfiesConstraints(
    T m, const T::value_type& value) const {
  if (!universe_) {
    return MisconfiguredError(Typename(), "SatisfiesConstraints",
                              InternalConfigurationType::kUniverse);
  }

  moriarty_internal::MVariableManager(&m).SetUniverse(
      universe_, absl::Substitute("SatisfiesConstraints::$0", m.Typename()));
  return moriarty_internal::MVariableManager(&m).IsSatisfiedWith(value);
}

template <typename V, typename G>
template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> MVariable<V, G>::Read(
    absl::string_view debug_name, T m) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "Read",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetIOConfig()) {
    return MisconfiguredError(Typename(), "Read",
                              InternalConfigurationType::kInputStream);
  }

  moriarty_internal::MVariableManager(&m).SetUniverse(
      universe_,
      /* my_name_in_universe = */ moriarty_internal::ConstructVariableName(
          variable_name_inside_universe_, debug_name));
  return moriarty_internal::MVariableManager(&m).TryRead();
}

template <typename V, typename G>
template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::Status MVariable<V, G>::Print(absl::string_view debug_name, T m,
                                    const T::value_type& value) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "Print",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetIOConfig()) {
    return MisconfiguredError(Typename(), "Print",
                              InternalConfigurationType::kOutputStream);
  }

  moriarty_internal::MVariableManager(&m).SetUniverse(
      universe_,
      /* my_name_in_universe = */ moriarty_internal::ConstructVariableName(
          variable_name_inside_universe_, debug_name));
  return moriarty_internal::MVariableManager(&m).TryPrint(value);
}

template <typename V, typename G>
template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> MVariable<V, G>::GetKnownValue(
    absl::string_view variable_name) const {
  if (!universe_) {
    return MisconfiguredError(Typename(), "GetKnownValue",
                              InternalConfigurationType::kUniverse);
  }

  return universe_->GetValue<T>(variable_name);
}

template <typename V, typename G>
template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> MVariable<V, G>::GenerateValue(
    absl::string_view variable_name) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "GenerateValue",
                              InternalConfigurationType::kUniverse);
  }

  return universe_->GetOrGenerateAndSetValue<T>(variable_name);
}

template <typename V, typename G>
template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
std::optional<typename T::value_type> MVariable<V, G>::GetUniqueValue(
    absl::string_view debug_name, T m) const {
  if (!universe_) return std::nullopt;

  moriarty_internal::MVariableManager(&m).SetUniverse(
      universe_, /* my_name_in_universe = */ debug_name);

  std::optional<std::any> value =
      moriarty_internal::MVariableManager(&m).GetUniqueValueUntyped();

  if (!value) return std::nullopt;

  typename T::value_type* typed_value =
      std::any_cast<typename T::value_type>(&(*value));

  // This should not happen. It means the MVariable is returning the
  // wrong type from GetUniqueValue(). It must be exactly the right type.
  ABSL_CHECK_NE(typed_value, nullptr)
      << "Unable to cast value from GetUniqueValue to the appropriate type.";

  return *typed_value;
}

template <typename V, typename G>
absl::StatusOr<int64_t> MVariable<V, G>::RandomInteger(int64_t min,
                                                       int64_t max) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "RandomInteger",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetRandomEngine()) {
    return MisconfiguredError(Typename(), "RandomInteger",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomInteger(*universe_->GetRandomEngine(), min,
                                          max);
}

template <typename V, typename G>
absl::StatusOr<int64_t> MVariable<V, G>::RandomInteger(int64_t n) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "RandomInteger",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetRandomEngine()) {
    return MisconfiguredError(Typename(), "RandomInteger",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomInteger(*universe_->GetRandomEngine(), n);
}

template <typename V, typename G>
template <typename T>
absl::Status MVariable<V, G>::Shuffle(std::vector<T>& container) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "RandomInteger",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetRandomEngine()) {
    return MisconfiguredError(Typename(), "RandomInteger",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::Shuffle(*universe_->GetRandomEngine(), container);
}

template <typename V, typename G>
template <typename T>
absl::StatusOr<T> MVariable<V, G>::RandomElement(
    const std::vector<T>& container) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "RandomElement",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetRandomEngine()) {
    return MisconfiguredError(Typename(), "RandomElement",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomElement(*universe_->GetRandomEngine(),
                                          container);
}

template <typename V, typename G>
template <typename T>
absl::StatusOr<std::vector<T>> MVariable<V, G>::RandomElementsWithReplacement(
    const std::vector<T>& container, int k) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "RandomElementsWithReplacement",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetRandomEngine()) {
    return MisconfiguredError(Typename(), "RandomElementsWithReplacement",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomElementsWithReplacement(
      *universe_->GetRandomEngine(), container, k);
}

template <typename V, typename G>
template <typename T>
absl::StatusOr<std::vector<T>>
MVariable<V, G>::RandomElementsWithoutReplacement(
    const std::vector<T>& container, int k) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "RandomElementsWithoutReplacement",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetRandomEngine()) {
    return MisconfiguredError(Typename(), "RandomElementsWithoutReplacement",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomElementsWithoutReplacement(
      *universe_->GetRandomEngine(), container, k);
}

template <typename V, typename G>
absl::StatusOr<std::vector<int>> MVariable<V, G>::RandomPermutation(int n) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "RandomPermutation",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetRandomEngine()) {
    return MisconfiguredError(Typename(), "RandomPermutation",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomPermutation(*universe_->GetRandomEngine(), n);
}

template <typename V, typename G>
template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> MVariable<V, G>::RandomPermutation(int n,
                                                                  T min) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "RandomPermutation",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetRandomEngine()) {
    return MisconfiguredError(Typename(), "RandomPermutation",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomPermutation(*universe_->GetRandomEngine(), n,
                                              min);
}

template <typename V, typename G>
template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> MVariable<V, G>::DistinctIntegers(T n, int k,
                                                                 T min) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "DistinctIntegers",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetRandomEngine()) {
    return MisconfiguredError(Typename(), "DistinctIntegers",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::DistinctIntegers(*universe_->GetRandomEngine(), n,
                                             k, min);
}

template <typename V, typename G>
template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> MVariable<V, G>::RandomComposition(
    T n, int k, T min_bucket_size) {
  if (!universe_) {
    return MisconfiguredError(Typename(), "RandomComposition",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetRandomEngine()) {
    return MisconfiguredError(Typename(), "RandomComposition",
                              InternalConfigurationType::kRandomEngine);
  }

  return moriarty_internal::RandomComposition(*universe_->GetRandomEngine(), n,
                                              k, min_bucket_size);
}

template <typename V, typename G>
absl::StatusOr<absl::Nonnull<const IOConfig*>> MVariable<V, G>::GetIOConfig()
    const {
  if (!universe_) {
    return MisconfiguredError(Typename(), "GetIOConfig",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetIOConfig()) {
    return MisconfiguredError(Typename(), "GetIOConfig",
                              InternalConfigurationType::kIOConfig);
  }
  return universe_->GetIOConfig();
}

template <typename V, typename G>
absl::StatusOr<absl::Nonnull<IOConfig*>> MVariable<V, G>::GetIOConfig() {
  if (!universe_) {
    return MisconfiguredError(Typename(), "GetIOConfig",
                              InternalConfigurationType::kUniverse);
  }
  return universe_->GetIOConfig();
}

template <typename V, typename G>
std::optional<int64_t> MVariable<V, G>::GetApproximateGenerationLimit() const {
  if (!universe_) return std::nullopt;
  if (!universe_->GetGenerationConfig()) return std::nullopt;

  return universe_->GetGenerationConfig()->GetSoftGenerationLimit();
}

template <typename V, typename G>
template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
std::vector<std::string> MVariable<V, G>::GetDependencies(T variable) const {
  return moriarty_internal::MVariableManager(&variable).GetDependencies();
}

// -----------------------------------------------------------------------------
//  Template implementation for the Internal Extended API

template <typename V, typename G>
absl::StatusOr<G> MVariable<V, G>::Generate() {
  if (!universe_) {
    return MisconfiguredError(Typename(), "Generate",
                              InternalConfigurationType::kUniverse);
  }
  if (!universe_->GetGenerationConfig()) {
    return MisconfiguredError(Typename(), "Generate",
                              InternalConfigurationType::kGenerationConfig);
  }
  if (!universe_->GetRandomEngine()) {
    return MisconfiguredError(Typename(), "Generate",
                              InternalConfigurationType::kRandomEngine);
  }

  if (is_one_of_ && is_one_of_->empty())
    return absl::FailedPreconditionError(
        "Is/IsOneOf used, but no viable value found.");

  moriarty_internal::GenerationConfig& generation_config =
      *universe_->GetGenerationConfig();

  MORIARTY_RETURN_IF_ERROR(
      generation_config.MarkStartGeneration(variable_name_inside_universe_));

  while (true) {
    absl::StatusOr<G> value = GenerateOnce();
    if (value.ok()) {
      MORIARTY_RETURN_IF_ERROR(generation_config.MarkSuccessfulGeneration(
          variable_name_inside_universe_));
      return value;
    }

    // TODO(darcybest): If value.status is not a Moriarty error, we should stop.

    MORIARTY_ASSIGN_OR_RETURN(
        moriarty_internal::GenerationConfig::RetryRecommendation
            retry_recommendation,
        generation_config.AddGenerationFailure(variable_name_inside_universe_,
                                               value.status()));

    for (absl::string_view variable_name :
         retry_recommendation.variable_names_to_delete) {
      MORIARTY_RETURN_IF_ERROR(universe_->EraseValue(variable_name));
    }

    if (retry_recommendation.policy ==
        moriarty_internal::GenerationConfig::RetryRecommendation::kAbort) {
      break;
    }
  }

  MORIARTY_RETURN_IF_ERROR(generation_config.MarkAbandonedGeneration(
      variable_name_inside_universe_));

  return absl::FailedPreconditionError(absl::Substitute(
      "Error generating '$0' (even with retries). One such error: $1",
      variable_name_inside_universe_,
      generation_config.GetGenerationStatus(variable_name_inside_universe_)
          .ToString()));
}

template <typename V, typename G>
absl::Status MVariable<V, G>::IsSatisfiedWith(const G& value) const {
  if (!universe_) {
    return MisconfiguredError(Typename(), "IsSatisfiedWith",
                              InternalConfigurationType::kUniverse);
  }

  if (is_one_of_) {
    MORIARTY_RETURN_IF_ERROR(CheckConstraint(
        absl::c_binary_search(*is_one_of_, value),
        "`value` must be one of the options in Is() and IsOneOf()"));
  }

  absl::Status status = IsSatisfiedWithImpl(value);
  if (!status.ok()) {
    if (IsVariableNotFoundError(status) || IsValueNotFoundError(status))
      return UnsatisfiedConstraintError(status.message());
    return status;
  }

  ConstraintValues cv(universe_);
  for (const auto& [checker_name, checker] : custom_constraints_) {
    MORIARTY_RETURN_IF_ERROR(CheckConstraint(
        checker(value, cv),
        absl::Substitute("Custom constraint '$0' not satisfied.",
                         checker_name)));
  }

  return absl::OkStatus();
}

template <typename V, typename G>
absl::Status MVariable<V, G>::MergeFrom(
    const moriarty_internal::AbstractVariable& other) {
  const V* const other_derived_class = dynamic_cast<const V* const>(&other);

  if (other_derived_class == nullptr)
    return absl::InvalidArgumentError(
        "In MergeFrom: Cannot convert variable to this variable type.");

  return TryMergeFrom(*other_derived_class);
}

template <typename V, typename G>
absl::StatusOr<G> MVariable<V, G>::TryRead() {
  return ReadImpl();
}

template <typename V, typename G>
absl::Status MVariable<V, G>::TryPrint(const G& value) {
  return PrintImpl(value);
}

template <typename V, typename G>
absl::StatusOr<std::any> MVariable<V, G>::GetSubvalue(
    const std::any& my_value, absl::string_view subvalue_name) const {
  const G val = std::any_cast<const G>(my_value);
  MORIARTY_ASSIGN_OR_RETURN(Subvalues subvalues, GetSubvaluesImpl(val));

  MORIARTY_ASSIGN_OR_RETURN(
      const moriarty_internal::VariableValue* subvalue,
      moriarty_internal::SubvaluesManager(&subvalues)
          .GetSubvalue(moriarty_internal::BaseVariableName(subvalue_name)));

  if (!moriarty_internal::HasSubvariable(subvalue_name)) return subvalue->value;

  // Safe * on optional SubvariableName since HasSubVariable() is true.
  return subvalue->variable->GetSubvalue(
      subvalue->value, *moriarty_internal::SubvariableName(subvalue_name));
}

template <typename V, typename G>
moriarty_internal::RandomEngine& MVariable<V, G>::GetRandomEngine() {
  ABSL_CHECK_NE(universe_, nullptr)
      << "GetRandomEngine() called without a Universe";
  return *universe_->GetRandomEngine();
}

template <typename V, typename G>
void MVariable<V, G>::SetUniverse(moriarty_internal::Universe* universe,
                                  absl::string_view my_name_in_universe) {
  universe_ = universe;
  variable_name_inside_universe_ = my_name_in_universe;
}

// -----------------------------------------------------------------------------
//  Template implementation for private functions not part of Extended API.

template <typename V, typename G>
V& MVariable<V, G>::UnderlyingVariableType() {
  return static_cast<V&>(*this);
}

template <typename V, typename G>
const V& MVariable<V, G>::UnderlyingVariableType() const {
  return static_cast<const V&>(*this);
}

template <typename V, typename G>
std::unique_ptr<moriarty_internal::AbstractVariable> MVariable<V, G>::Clone()
    const {
  // Use V's copy constructor.
  return std::make_unique<V>(UnderlyingVariableType());
}

template <typename V, typename G>
absl::Status MVariable<V, G>::WithProperty(Property property) {
  return TryWithKnownProperty(std::move(property));
}

template <typename V, typename G>
absl::StatusOr<G> MVariable<V, G>::GenerateOnce() {
  MORIARTY_ASSIGN_OR_RETURN(G potential_value, [this]() -> absl::StatusOr<G> {
    if (is_one_of_) {
      return RandomElement(*is_one_of_);
    }
    return GenerateImpl();
  }());

  // Generate dependent variables used in custom constraints.
  for (const std::string& dep : custom_constraints_deps_) {
    MORIARTY_ASSIGN_OR_RETURN(AbstractVariable * var,
                              universe_->GetAbstractVariable(dep));
    MORIARTY_RETURN_IF_ERROR(var->AssignValue());
  }

  absl::Status satisfies = IsSatisfiedWith(potential_value);
  if (!satisfies.ok()) return satisfies;

  return potential_value;
}

template <typename V, typename G>
absl::Status MVariable<V, G>::AssignValue() {
  if (!universe_) {
    return MisconfiguredError(Typename(), "AssignValue",
                              InternalConfigurationType::kUniverse);
  }

  if (universe_->ValueIsKnown(variable_name_inside_universe_))
    return absl::OkStatus();

  MORIARTY_ASSIGN_OR_RETURN(G value, Generate());
  return universe_->SetValue<V>(variable_name_inside_universe_, value);
}

template <typename V, typename G>
absl::Status MVariable<V, G>::AssignUniqueValue() {
  if (!universe_) {
    return MisconfiguredError(Typename(), "AssignUniqueValue",
                              InternalConfigurationType::kUniverse);
  }

  if (universe_->ValueIsKnown(variable_name_inside_universe_))
    return absl::OkStatus();

  std::optional<std::any> value = GetUniqueValueUntyped();

  if (!value) return absl::OkStatus();

  G* typed_value = std::any_cast<G>(&(*value));
  if (typed_value == nullptr) {
    // This should not happen. It means the MVariable is returning the
    // wrong type from GetUniqueValueUntyped(). It must be exactly the right
    // type.
    return absl::InternalError(
        "Unable to cast value from GetUniqueValueUntyped to the correct type.");
  }

  return universe_->SetValue<V>(variable_name_inside_universe_, *typed_value);
}

template <typename V, typename G>
std::optional<std::any> MVariable<V, G>::GetUniqueValueUntyped() const {
  if (is_one_of_) {
    if (is_one_of_->size() == 1) return is_one_of_->at(0);
    return std::nullopt;  // Not sure which one is correct.
  }

  // Casting std::optional<G> to std::optional<std::any>.
  std::optional<G> value = GetUniqueValueImpl();
  if (!value) return std::nullopt;
  return *value;
}

template <typename V, typename G>
absl::Status MVariable<V, G>::ValueSatisfiesConstraints() const {
  if (!universe_) {
    return MisconfiguredError(Typename(), "ValueSatisfiesConstraints",
                              InternalConfigurationType::kUniverse);
  }

  MORIARTY_ASSIGN_OR_RETURN(
      G value, universe_->GetValue<V>(variable_name_inside_universe_));
  return IsSatisfiedWith(value);
}

template <typename V, typename G>
absl::Status MVariable<V, G>::ReadValue() {
  if (!universe_) {
    return MisconfiguredError(Typename(), "ReadValue",
                              InternalConfigurationType::kUniverse);
  }

  MORIARTY_ASSIGN_OR_RETURN(
      G value, TryRead(),
      _ << "failed to read " << variable_name_inside_universe_);
  return universe_->SetValue<V>(variable_name_inside_universe_,
                                std::move(value));
}

template <typename V, typename G>
absl::Status MVariable<V, G>::PrintValue() {
  if (!universe_) {
    return MisconfiguredError(Typename(), "PrintValue",
                              InternalConfigurationType::kUniverse);
  }

  MORIARTY_ASSIGN_OR_RETURN(
      G value, universe_->GetValue<V>(variable_name_inside_universe_));
  return TryPrint(value);
}

template <typename V, typename G>
std::vector<std::string> MVariable<V, G>::GetDependencies() {
  std::vector<std::string> this_deps = GetDependenciesImpl();
  absl::c_copy(custom_constraints_deps_, std::back_inserter(this_deps));
  return this_deps;
}

template <typename V, typename G>
absl::StatusOr<
    std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>>
MVariable<V, G>::GetDifficultAbstractVariables() const {
  MORIARTY_ASSIGN_OR_RETURN(std::vector<V> instances,
                            this->GetDifficultInstances());
  std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>> new_vec;
  new_vec.reserve(instances.size());
  for (V& instance : instances) {
    new_vec.push_back(std::make_unique<V>(std::move(instance)));
  }
  return new_vec;
}

}  // namespace librarian

namespace moriarty_internal {

template <typename VariableType, typename ValueType>
MVariableManager<VariableType, ValueType>::MVariableManager(
    moriarty::librarian::MVariable<VariableType, ValueType>*
        mvariable_to_manage)
    : managed_mvariable_(*mvariable_to_manage) {}

template <typename VariableType, typename ValueType>
absl::StatusOr<ValueType>
MVariableManager<VariableType, ValueType>::Generate() {
  return managed_mvariable_.Generate();
}

template <typename VariableType, typename ValueType>
absl::Status MVariableManager<VariableType, ValueType>::IsSatisfiedWith(
    const ValueType& value) const {
  return managed_mvariable_.IsSatisfiedWith(value);
}

template <typename VariableType, typename ValueType>
absl::Status MVariableManager<VariableType, ValueType>::MergeFrom(
    const moriarty_internal::AbstractVariable& other) {
  return managed_mvariable_.MergeFrom(other);
}

template <typename VariableType, typename ValueType>
absl::StatusOr<ValueType> MVariableManager<VariableType, ValueType>::TryRead() {
  return managed_mvariable_.TryRead();
}

template <typename VariableType, typename ValueType>
absl::Status MVariableManager<VariableType, ValueType>::TryPrint(
    const ValueType& value) {
  return managed_mvariable_.TryPrint(value);
}

template <typename VariableType, typename ValueType>
absl::StatusOr<std::any> MVariableManager<VariableType, ValueType>::GetSubvalue(
    const std::any& my_value, absl::string_view subvalue_name) {
  return managed_mvariable_.GetSubvalue(my_value, subvalue_name);
}

template <typename VariableType, typename ValueType>
moriarty_internal::RandomEngine&
MVariableManager<VariableType, ValueType>::GetRandomEngine() {
  return managed_mvariable_.GetRandomEngine();
}

template <typename VariableType, typename ValueType>
void MVariableManager<VariableType, ValueType>::SetUniverse(
    moriarty_internal::Universe* universe,
    absl::string_view my_name_in_universe) {
  managed_mvariable_.SetUniverse(universe, my_name_in_universe);
}

template <typename VariableType, typename ValueType>
std::optional<std::any>
MVariableManager<VariableType, ValueType>::GetUniqueValueUntyped() const {
  return managed_mvariable_.GetUniqueValueUntyped();
}

template <typename VariableType, typename ValueType>
std::vector<std::string>
MVariableManager<VariableType, ValueType>::GetDependencies() const {
  return managed_mvariable_.GetDependencies();
}

}  // namespace moriarty_internal

}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_MVARIABLE_H_
