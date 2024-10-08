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

#ifndef MORIARTY_SRC_VARIABLES_MINTEGER_H_
#define MORIARTY_SRC_VARIABLES_MINTEGER_H_

#include <concepts>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/internal/range.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/size_property.h"
#include "src/property.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/constraints/size_constraints.h"

namespace moriarty {

// MInteger
//
// Describes constraints placed on an integer. We mean a "mathematical" integer,
// not a "computer science" integer. As such, we intentionally do not have an
// MVariable for 32-bit integers.
class MInteger : public librarian::MVariable<MInteger, int64_t> {
 public:
  // Create an MInteger from a set of constraints. Logically equivalent to
  // calling AddConstraint() for each constraint.
  //
  // E.g., MInteger(Between(1, "3 * N + 1"), SizeCategory::Small())
  template <typename... Constraints>
    requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
  explicit MInteger(Constraints&&... constraints);

  // The integer must be exactly this value.
  MInteger& AddConstraint(const Exactly<int64_t>& constraint);
  // The integer must be exactly this integer expression (e.g., "3 * N + 1").
  MInteger& AddConstraint(const Exactly<std::string>& constraint);
  // The integer must be in this inclusive range.
  MInteger& AddConstraint(const Between& constraint);
  // The integer must be this value or smaller.
  MInteger& AddConstraint(const AtMost& constraint);
  // The integer must be this value or larger.
  MInteger& AddConstraint(const AtLeast& constraint);
  // The integer should be approximately this size.
  MInteger& AddConstraint(const SizeCategory& constraint);

  [[nodiscard]] std::string Typename() const override { return "MInteger"; }

  // Is()
  //
  // Restricts this integer to be exactly a specific integer.
  using librarian::MVariable<MInteger, int64_t>::Is;

  // Is()
  //
  // Restrict this integer to be exactly `integer_expression`.
  MInteger& Is(absl::string_view integer_expression);

  // Between()
  //
  // Restrict this integer to be in the inclusive range [`minimum`,
  // `maximum`]. If this restriction is called multiple times, the ranges are
  // intersected together. For example `x.Between(1, 10).Between(5, 12);` is
  // equivalent to `x.Between(5, 10);`.
  MInteger& Between(int64_t minimum, int64_t maximum);

  // Between()
  //
  // Restrict this integer to be in the inclusive range
  // [`minimum_integer_expression`, `maximum_integer_expression`].
  //
  // This integer expression may contain variable names, which should be in
  // the same global context as this variable (e.g., same Moriarty instance,
  // same Generator, etc)
  //
  // If any `.Between()` restriction is called multiple times, the ranges are
  // intersected together. For example `x.Between(1, 10).Between(5, "3 * N");`
  // is equivalent to `x.Between(5, "min(10, 3 * N)");`.
  MInteger& Between(absl::string_view minimum_integer_expression,
                    absl::string_view maximum_integer_expression);

  // Between()
  //
  // Restrict this integer to be in the inclusive range
  // [`minimum`, `maximum_integer_expression`].
  //
  // See above for further clarifications.
  MInteger& Between(int64_t minimum,
                    absl::string_view maximum_integer_expression);

  // Between()
  //
  // Restricts this integer to be in the inclusive range
  // [`minimum_integer_expression`, `maximum`].
  //
  // See above for further clarifications.
  MInteger& Between(absl::string_view minimum_integer_expression,
                    int64_t maximum);

  // AtLeast()
  //
  // Restricts this integer to be larger than or equal to `minimum`.
  MInteger& AtLeast(int64_t minimum);

  // AtLeast()
  //
  // Restricts this integer to be larger than or equal to
  // `minimum_integer_expression`.
  //
  // This integer expression may contain variable names, which should be in
  // the same global context as this variable (e.g., same Moriarty instance,
  // same Generator, etc)
  MInteger& AtLeast(absl::string_view minimum_integer_expression);

  // AtMost()
  //
  // Restricts this integer to be larger than or equal to `maximum`.
  MInteger& AtMost(int64_t maximum);

  // AtMost()
  //
  // Restricts this integer to be larger than or equal to
  // `maximum_integer_expression`.
  //
  // This integer expression may contain variable names, which should be in
  // the same global context as this variable (e.g., same Moriarty instance,
  // same Generator, etc)
  MInteger& AtMost(absl::string_view maximum_integer_expression);

  // WithSize()
  //
  // Sets the approximate size of this integer.
  // The exact meaning of each of these is an implementation detail, but they
  // are approximately in the order specified here (for example, in general,
  // you should expect "tiny" to be smaller than "small", and "small" to be
  // smaller than "medium", etc.)
  MInteger& WithSize(CommonSize size);

  // OfSizeProperty()
  //
  // Tells this int to have a specific size. `property.category` must be
  // "size". The exact values here are not guaranteed and may change over
  // time. If exact values are required, specify them manually.
  absl::Status OfSizeProperty(Property property);

 private:
  Range bounds_;

  // What approximate size should the int64_t be when it is generated.
  CommonSize approx_size_ = CommonSize::kAny;

  // Computes and returns the minimum and maximum of `bounds_`. Returns
  // `kInvalidArgumentError` if the range is empty. The `non-const` version
  // may generate other dependent variables if needed along the way.
  absl::StatusOr<Range::ExtremeValues> GetExtremeValues() const;
  absl::StatusOr<Range::ExtremeValues> GetExtremeValues();

  // Generates a value between `minimum` and `maximum`.
  absl::StatusOr<int64_t> GenerateInRange(Range::ExtremeValues extremes);

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  absl::StatusOr<int64_t> GenerateImpl() override;
  absl::Status IsSatisfiedWithImpl(const int64_t& value) const override;
  absl::Status MergeFromImpl(const MInteger& other) override;
  absl::StatusOr<int64_t> ReadImpl() override;
  absl::Status PrintImpl(const int64_t& value) override;
  std::vector<std::string> GetDependenciesImpl() const override;
  absl::StatusOr<std::vector<MInteger>> GetDifficultInstancesImpl()
      const override;
  std::optional<int64_t> GetUniqueValueImpl() const override;
  std::string ToStringImpl() const override;
  absl::StatusOr<std::string> ValueToStringImpl(
      const int64_t& value) const override;
  // ---------------------------------------------------------------------------
};

// -----------------------------------------------------------------------------
//  Implementation details
// -----------------------------------------------------------------------------

template <typename... Constraints>
  requires(std::derived_from<std::decay_t<Constraints>, MConstraint> && ...)
MInteger::MInteger(Constraints&&... constraints) {
  RegisterKnownProperty("size", &MInteger::OfSizeProperty);
  (AddConstraint(std::forward<Constraints>(constraints)), ...);
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MINTEGER_H_
