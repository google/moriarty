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

#ifndef MORIARTY_SRC_VARIABLES_MARRAY_H_
#define MORIARTY_SRC_VARIABLES_MARRAY_H_

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/property.h"
#include "src/util/status_macro/status_macros.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/container_constraints.h"
#include "src/variables/constraints/io_constraints.h"
#include "src/variables/constraints/size_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {

// MArray<>
//
// Moriarty's array type. An MArray<MElementType> specifies that you want to
// create an array of ElementType's. The constraints on each element are
// controlled by the MElementType class. All elements have the same constraints.
template <typename MElementType>
class MArray : public librarian::MVariable<
                   MArray<MElementType>,
                   std::vector<typename MElementType::value_type>> {
 public:
  using element_value_type = typename MElementType::value_type;
  using vector_value_type = typename std::vector<element_value_type>;

  // Create an MArray from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint. E.g.,
  // MArray<MInteger>(Elements<MInteger>(Between(1, 10)), Length(15))
  template <typename... Constraints>
    requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
  explicit MArray(Constraints&&... constraints);

  // Create an MArray with this set of constraints on each element.
  explicit MArray(MElementType element_constraints);

  // The array must have exactly this value.
  MArray& AddConstraint(const Exactly<vector_value_type>& constraint);
  // The array's elements must satisfy these constraints.
  MArray& AddConstraint(const Elements<MElementType>& constraint);
  // The array must have this length.
  MArray& AddConstraint(const Length& constraint);
  // The array's elements must be distinct.
  MArray& AddConstraint(const DistinctElements& constraint);
  // The array's elements must be separated by this whitespace.
  MArray& AddConstraint(const IOSeparator& constraint);
  // The array should be approximately this size.
  MArray& AddConstraint(const SizeCategory& constraint);

  // Typename()
  //
  // Returns a string representing the name of this type (for example,
  // "MArray<MInteger>"). This is mostly used for debugging/error messages.
  [[nodiscard]] std::string Typename() const override;

  // Of()
  //
  // Add extra constraints to the elements of the array.
  MArray& Of(MElementType variable);

  // OfLength()
  //
  // Sets the constraints for the length of this array. If two parameters are
  // provided, the length is in the closed range [`min_length`, `max_length`].
  //
  // For example:
  // `OfLength(5, 10)` is equivalent to `OfLength(MInteger().Between(5, 10))`.
  // `OfLength("3 * N")` is equivalent to `OfLength(MInteger().Is("3 * N"))`.
  MArray& OfLength(const MInteger& length);
  MArray& OfLength(int64_t length);
  MArray& OfLength(absl::string_view length_expression);
  MArray& OfLength(int64_t min_length, int64_t max_length);
  MArray& OfLength(int64_t min_length, absl::string_view max_length_expression);
  MArray& OfLength(absl::string_view min_length_expression, int64_t max_length);
  MArray& OfLength(absl::string_view min_length_expression,
                   absl::string_view max_length_expression);

  // WithDistinctElements()
  //
  // States that this array must contain only distinct elements. By default,
  // this restriction is off, and does not guarantee duplicate entries will
  // occur.
  MArray& WithDistinctElements();

  // WithDistinctElementsWithArg()
  //
  // DEPRECATED. Do not use. Use WithDistinctElements() instead.
  [[deprecated("Use WithDistinctElements() instead.")]] MArray&
  WithDistinctElementsWithArg(bool distinct_elements = true);

  // WithSeparator()
  //
  // Sets the whitespace separator to be used between different array entries
  // when reading/writing. Default = kSpace.
  MArray& WithSeparator(Whitespace separator);

  // OfSizeProperty()
  //
  // Tells this string to have a specific size. `property.category` must
  // be "size". The exact values here are not guaranteed and may change over
  // time. If exact values are required, specify them manually.
  //
  // Note that "size" and "length" are different topics. The "length" is the
  // number of elements in the array, while the "size" is a subjective measure
  // -- "small", "medium", "large", etc.
  absl::Status OfSizeProperty(Property property);

 private:
  static constexpr const int kGenerateRetries = 100;

  MElementType element_constraints_;
  std::optional<MInteger> length_;
  bool distinct_elements_ = false;
  // TODO(hivini): Find a way to improve this. Like using StatusOr<> instead.
  absl::Status separator_status_ = absl::OkStatus();
  std::optional<Whitespace> separator_;

  std::optional<Property> length_size_property_;

  // GenerateNDistinctImpl()
  //
  // Same as GenerateImpl(), but guarantees that all elements are distinct.
  absl::StatusOr<vector_value_type> GenerateNDistinctImpl(int n);

  // GenerateUnseenElement()
  //
  // Returns a element that is not in `seen`. `remaining_retries` is the maximum
  // number of times `Generate()` may be called. This function updates
  // `remaining_retries`.
  absl::StatusOr<element_value_type> GenerateUnseenElement(
      const absl::flat_hash_set<element_value_type>& seen,
      int& remaining_retries, int index);

  // GetNumberOfRetriesForDistinctElements()
  //
  // Returns the number of total calls to Generate() that should be done in
  // order to confidently generate `n` distinct elements. Exact probabilities
  // may change over time, but is aimed at <1% failure rate at the moment.
  static int GetNumberOfRetriesForDistinctElements(int n);

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  absl::StatusOr<vector_value_type> GenerateImpl() override;
  absl::Status IsSatisfiedWithImpl(
      const vector_value_type& value) const override;
  absl::Status MergeFromImpl(const MArray<MElementType>& other) override;
  absl::StatusOr<vector_value_type> ReadImpl() override;
  absl::Status PrintImpl(const vector_value_type& value) override;
  std::vector<std::string> GetDependenciesImpl() const override;
  absl::StatusOr<std::vector<MArray<MElementType>>> GetDifficultInstancesImpl()
      const override;
  // ---------------------------------------------------------------------------
};

// Class template argument deduction (CTAD). Allows for `MArray(MInteger())`
// instead of `MArray<MInteger>(MInteger())`. See `NestedMArray()` for nesting
// multiple `MArrays` inside one another.
template <typename MoriartyElementType>
MArray(MoriartyElementType) -> MArray<MoriartyElementType>;

// NestedMArray()
//
// Returns an array to `elements`.
//
// CTAD does not allow for nested `MArray`s. `MArray(MArray(MInteger())) is
// interpreted as `MArray<MInteger>(MInteger())`, since the outer `MArray` is
// calling the copy constructor.
//
// Examples:
//   `NestedMArray(MArray(MInteger()))`
//   `NestedMArray(NestedMArray(MArray(MInteger())))`
//   `NestedMArray(NestedMArray(MInteger()))`
template <typename MElementType, typename... Constraints>
  requires std::constructible_from<MArray<MElementType>, Constraints...>
MArray<MArray<MElementType>> NestedMArray(MArray<MElementType> elements,
                                          Constraints&&... constraints) {
  MArray<MArray<MElementType>> res(std::move(elements));
  (res.AddConstraint(std::forward<Constraints>(constraints)), ...);
  return res;
}

// -----------------------------------------------------------------------------
//  Template Implementation Below

template <typename MElementType>
MArray<MElementType>::MArray(MElementType element_constraints)
    : MArray<MElementType>(
          Elements<MElementType>(std::move(element_constraints))) {}

template <typename MElementType>
template <typename... Constraints>
  requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
MArray<MElementType>::MArray(Constraints&&... constraints) {
  static_assert(
      std::derived_from<MElementType,
                        librarian::MVariable<
                            MElementType, typename MElementType::value_type>>,
      "The T used in MArray<T> must be a Moriarty variable. For example, "
      "MArray<MInteger> or MArray<MCustomType>.");
  this->RegisterKnownProperty("size", &MArray<MElementType>::OfSizeProperty);
  (AddConstraint(std::forward<Constraints>(constraints)), ...);
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::AddConstraint(
    const Exactly<vector_value_type>& constraint) {
  return this->Is(constraint.GetValue());
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::AddConstraint(
    const Elements<MElementType>& constraint) {
  return Of(constraint.GetConstraints());
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::AddConstraint(
    const Length& constraint) {
  return OfLength(constraint.GetConstraints());
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::AddConstraint(
    const DistinctElements& constraint) {
  return WithDistinctElements();
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::AddConstraint(
    const IOSeparator& constraint) {
  return WithSeparator(constraint.GetSeparator());
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::AddConstraint(
    const SizeCategory& constraint) {
  return AddConstraint(Length(constraint));
}

template <typename MElementType>
std::string MArray<MElementType>::Typename() const {
  return absl::StrCat("MArray<", element_constraints_.Typename(), ">");
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::Of(MElementType variable) {
  element_constraints_.MergeFrom(std::move(variable));
  return *this;
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::OfLength(const MInteger& length) {
  if (length_)
    length_->MergeFrom(length);
  else
    length_ = length;
  return *this;
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::OfLength(int64_t length) {
  return OfLength(length, length);
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::OfLength(
    absl::string_view length_expression) {
  return OfLength(length_expression, length_expression);
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::OfLength(int64_t min_length,
                                                     int64_t max_length) {
  return OfLength(MInteger().Between(min_length, max_length));
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::OfLength(
    int64_t min_length, absl::string_view max_length_expression) {
  return OfLength(MInteger().Between(min_length, max_length_expression));
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::OfLength(
    absl::string_view min_length_expression, int64_t max_length) {
  return OfLength(MInteger().Between(min_length_expression, max_length));
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::OfLength(
    absl::string_view min_length_expression,
    absl::string_view max_length_expression) {
  return OfLength(
      MInteger().Between(min_length_expression, max_length_expression));
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::WithDistinctElements() {
  distinct_elements_ = true;
  return *this;
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::WithDistinctElementsWithArg(
    bool distinct_elements) {
  distinct_elements_ = distinct_elements;
  return *this;
}

template <typename MElementType>
MArray<MElementType>& MArray<MElementType>::WithSeparator(
    Whitespace separator) {
  if (!separator_status_.ok()) return *this;

  if (separator_.has_value() && *separator_ != separator) {
    separator_status_ = absl::Status(absl::StatusCode::kFailedPrecondition,
                                     "Invalid MArray separator state. Only "
                                     "one type of separator is supported.");
  } else {
    separator_ = separator;
  }
  return *this;
}

template <typename MElementType>
absl::Status MArray<MElementType>::MergeFromImpl(
    const MArray<MElementType>& other) {
  Of(other.element_constraints_);
  if (other.length_) OfLength(*other.length_);

  return absl::OkStatus();
}

template <typename MElementType>
auto MArray<MElementType>::GenerateImpl() -> absl::StatusOr<vector_value_type> {
  if (!length_)
    return absl::FailedPreconditionError(
        "Attempting to generate an array with no length parameter given.");

  // Ensure that the size is non-negative.
  length_->AtLeast(0);

  if (length_size_property_) {
    MORIARTY_RETURN_IF_ERROR(length_->OfSizeProperty(*length_size_property_));
  }

  std::optional<int64_t> generation_limit =
      this->GetApproximateGenerationLimit();

  if (generation_limit) {
    length_->AtMost(*generation_limit);
  }

  MORIARTY_ASSIGN_OR_RETURN(int length, this->Random("length", *length_));

  if (distinct_elements_) return GenerateNDistinctImpl(length);

  vector_value_type res;
  res.reserve(length);

  for (int i = 0; i < length; i++) {
    // `auto` is the type generated by element_constraints_
    MORIARTY_ASSIGN_OR_RETURN(
        auto value,
        this->Random(absl::StrCat("element[", i, "]"), element_constraints_));
    res.push_back(value);
  }

  return res;
}

template <typename MElementType>
auto MArray<MElementType>::GenerateNDistinctImpl(int n)
    -> absl::StatusOr<vector_value_type> {
  vector_value_type res;  // Do not res.reserve(n) in case n is massive.

  absl::flat_hash_set<element_value_type> values_seen;
  int remaining_retries = GetNumberOfRetriesForDistinctElements(n);

  for (int i = 0; i < n && remaining_retries > 0; i++) {
    MORIARTY_ASSIGN_OR_RETURN(
        element_value_type value,
        GenerateUnseenElement(values_seen, remaining_retries, i));
    res.push_back(value);
    values_seen.insert(value);
  }

  return res;
}

template <typename MElementType>
auto MArray<MElementType>::GenerateUnseenElement(
    const absl::flat_hash_set<element_value_type>& seen, int& remaining_retries,
    int index) -> absl::StatusOr<element_value_type> {
  for (; remaining_retries > 0; remaining_retries--) {
    MORIARTY_ASSIGN_OR_RETURN(element_value_type value,
                              this->Random(absl::StrCat("element[", index, "]"),
                                           element_constraints_));
    if (!seen.contains(value)) return value;
  }

  return absl::FailedPreconditionError(
      "Cannot generate enough distinct values for array.");
}

template <typename MElementType>
int MArray<MElementType>::GetNumberOfRetriesForDistinctElements(int n) {
  // The worst case is randomly generating n numbers between 1 and n.
  //
  //   T   := number of iterations to get all n values.
  //   H_n := Harmonic number (1/1 + 1/2 + ... + 1/n).
  //
  //      Prob(|T - n * H_n| > c * n) < pi^2 / (6 * c^2)
  //
  // Thus, if c = 14:
  //
  //      Prob(T > n * H_n + 14 * n) < 1%.
  double H_n = 0;
  for (int i = n; i >= 1; i--) H_n += 1.0 / i;

  return n * H_n + 14 * n;
}

template <typename MElementType>
absl::Status MArray<MElementType>::OfSizeProperty(Property property) {
  length_size_property_ = std::move(property);
  return absl::OkStatus();
}

template <typename MElementType>
std::vector<std::string> MArray<MElementType>::GetDependenciesImpl() const {
  std::vector<std::string> deps = this->GetDependencies(element_constraints_);
  if (length_)
    absl::c_move(this->GetDependencies(*length_), std::back_inserter(deps));
  return deps;
}

template <typename MoriartyElementType>
absl::Status MArray<MoriartyElementType>::PrintImpl(
    const vector_value_type& value) {
  for (int i = 0; i < value.size(); i++) {
    // TODO(b/208296530): Add different separators
    if (i > 0) {
      MORIARTY_ASSIGN_OR_RETURN(librarian::IOConfig * io_config,
                                this->GetIOConfig());
      MORIARTY_RETURN_IF_ERROR(separator_status_);
      MORIARTY_RETURN_IF_ERROR(io_config->PrintWhitespace(
          separator_.has_value() ? *separator_ : Whitespace::kSpace));
    }
    MORIARTY_RETURN_IF_ERROR(this->Print(absl::StrCat("element[", i, "]"),
                                         element_constraints_, value[i]));
  }
  return absl::OkStatus();
}

template <typename MoriartyElementType>
absl::StatusOr<typename MArray<MoriartyElementType>::vector_value_type>
MArray<MoriartyElementType>::ReadImpl() {
  if (!length_) {
    return absl::FailedPreconditionError(
        "Unknown length of array before read.");
  }
  std::optional<int64_t> length = this->GetUniqueValue("length", *length_);

  if (!length) {
    return absl::FailedPreconditionError(
        "Cannot determine the length of array before read.");
  }

  MORIARTY_ASSIGN_OR_RETURN(librarian::IOConfig * io_config,
                            this->GetIOConfig());
  vector_value_type res;
  res.reserve(*length);
  for (int i = 0; i < *length; i++) {
    if (i > 0) {
      MORIARTY_RETURN_IF_ERROR(separator_status_);
      MORIARTY_RETURN_IF_ERROR(io_config->ReadWhitespace(
          separator_.has_value() ? *separator_ : Whitespace::kSpace));
    }
    MORIARTY_ASSIGN_OR_RETURN(
        element_value_type elem,
        this->Read(absl::StrCat("element[", i, "]"), element_constraints_));
    res.push_back(std::move(elem));
  }
  return res;
}

template <typename MoriartyElementType>
absl::Status MArray<MoriartyElementType>::IsSatisfiedWithImpl(
    const vector_value_type& value) const {
  if (length_) {
    MORIARTY_RETURN_IF_ERROR(
        CheckConstraint(this->SatisfiesConstraints(*length_, value.size()),
                        "invalid MArray length"));
  }

  for (int i = 0; i < value.size(); i++) {
    MORIARTY_RETURN_IF_ERROR(CheckConstraint(
        this->SatisfiesConstraints(element_constraints_, value[i]),
        absl::Substitute("invalid element $0 (0-based)", i)));
  }

  if (distinct_elements_) {
    absl::flat_hash_set<element_value_type> seen;
    int idx = 0;
    for (const element_value_type& x : value) {
      auto [it, inserted] = seen.insert(x);
      MORIARTY_RETURN_IF_ERROR(CheckConstraint(
          inserted, absl::Substitute("elements are not distinct. Element at "
                                     "index $0 appears multiple times.",
                                     idx)));
      idx++;
    }
  }

  return absl::OkStatus();
}
template <typename MoriartyElementType>
absl::StatusOr<std::vector<MArray<MoriartyElementType>>>
MArray<MoriartyElementType>::GetDifficultInstancesImpl() const {
  if (!length_) {
    return absl::FailedPreconditionError(
        "Attempting to get difficult instances of an Array with no "
        "length parameter given.");
  }
  std::vector<MArray<MoriartyElementType>> cases;
  MORIARTY_ASSIGN_OR_RETURN(std::vector<MInteger> lengthCases,
                            length_->GetDifficultInstances());

  cases.reserve(lengthCases.size());
  for (const auto& c : lengthCases) {
    cases.push_back(MArray().OfLength(c));
  }
  // TODO(hivini): Add cases for sort and the difficult instances of the
  // variable it holds.
  return cases;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MARRAY_H_
