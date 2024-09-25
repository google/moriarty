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

#include "src/variables/minteger.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "src/errors.h"
#include "src/internal/random_engine.h"
#include "src/internal/range.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/size_property.h"
#include "src/property.h"
#include "src/util/status_macro/status_macros.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/constraints/size_constraints.h"

namespace moriarty {

using ::moriarty::librarian::IOConfig;

MInteger& MInteger::AddConstraint(const Exactly<int64_t>& constraint) {
  bounds_.Intersect(Range(constraint.GetValue(), constraint.GetValue()));
  return *this;
}

MInteger& MInteger::AddConstraint(const Exactly<std::string>& constraint) {
  Range r;
  if (absl::Status status = r.AtLeast(constraint.GetValue()); !status.ok()) {
    DeclareSelfAsInvalid(UnsatisfiedConstraintError(
        absl::StrCat("Exactly() ", status.message())));
    return *this;
  }
  if (absl::Status status = r.AtMost(constraint.GetValue()); !status.ok()) {
    DeclareSelfAsInvalid(UnsatisfiedConstraintError(
        absl::StrCat("Exactly() ", status.message())));
    return *this;
  }

  bounds_.Intersect(r);
  return *this;
}

MInteger& MInteger::AddConstraint(const class Between& constraint) {
  bounds_.Intersect(constraint.GetRange());
  return *this;
}

MInteger& MInteger::AddConstraint(const class AtMost& constraint) {
  bounds_.Intersect(constraint.GetRange());
  return *this;
}

MInteger& MInteger::AddConstraint(const class AtLeast& constraint) {
  bounds_.Intersect(constraint.GetRange());
  return *this;
}

MInteger& MInteger::AddConstraint(const SizeCategory& constraint) {
  std::optional<CommonSize> size =
      librarian::MergeSizes(approx_size_, constraint.GetCommonSize());
  if (size) {
    approx_size_ = *size;
  } else {
    DeclareSelfAsInvalid(UnsatisfiedConstraintError(
        absl::Substitute("Invalid size. Unable to be both $0 and $1.",
                         librarian::ToString(constraint.GetCommonSize()),
                         librarian::ToString(approx_size_))));
  }
  return *this;
}

MInteger& MInteger::Is(absl::string_view integer_expression) {
  return AddConstraint(Exactly(integer_expression));
}

MInteger& MInteger::Between(int64_t minimum, int64_t maximum) {
  return AddConstraint(::moriarty::Between(minimum, maximum));
}

MInteger& MInteger::Between(absl::string_view minimum_integer_expression,
                            absl::string_view maximum_integer_expression) {
  return AddConstraint(::moriarty::Between(minimum_integer_expression,
                                           maximum_integer_expression));
}

MInteger& MInteger::Between(int64_t minimum,
                            absl::string_view maximum_integer_expression) {
  return AddConstraint(
      ::moriarty::Between(minimum, maximum_integer_expression));
}

MInteger& MInteger::Between(absl::string_view minimum_integer_expression,
                            int64_t maximum) {
  return AddConstraint(
      ::moriarty::Between(minimum_integer_expression, maximum));
}

MInteger& MInteger::AtLeast(int64_t minimum) {
  return AddConstraint(::moriarty::AtLeast(minimum));
}

MInteger& MInteger::AtLeast(absl::string_view minimum_integer_expression) {
  return AddConstraint(::moriarty::AtLeast(minimum_integer_expression));
}

MInteger& MInteger::AtMost(int64_t maximum) {
  return AddConstraint(::moriarty::AtMost(maximum));
}

MInteger& MInteger::AtMost(absl::string_view maximum_integer_expression) {
  return AddConstraint(::moriarty::AtMost(maximum_integer_expression));
}

std::optional<int64_t> MInteger::GetUniqueValueImpl() const {
  absl::StatusOr<Range::ExtremeValues> extremes = GetExtremeValues();
  if (!extremes.ok()) return std::nullopt;
  if (extremes->min != extremes->max) return std::nullopt;

  return extremes->min;
}

absl::StatusOr<Range::ExtremeValues> MInteger::GetExtremeValues() {
  MORIARTY_ASSIGN_OR_RETURN(
      absl::flat_hash_set<std::string> needed_dependent_variables,
      bounds_.NeededVariables(), _ << "Error getting the needed variables");

  absl::flat_hash_map<std::string, int64_t> dependent_variables;

  for (absl::string_view name : needed_dependent_variables) {
    MORIARTY_ASSIGN_OR_RETURN(
        dependent_variables[name], GenerateValue<MInteger>(name),
        _ << "Error getting the dependent variable " << name);
  }

  MORIARTY_ASSIGN_OR_RETURN(std::optional<Range::ExtremeValues> extremes,
                            bounds_.Extremes(dependent_variables));
  if (!extremes) return absl::InvalidArgumentError("Valid range is empty");
  return *extremes;
}

absl::StatusOr<Range::ExtremeValues> MInteger::GetExtremeValues() const {
  MORIARTY_ASSIGN_OR_RETURN(
      absl::flat_hash_set<std::string> needed_dependent_variables,
      bounds_.NeededVariables(), _ << "Error getting the needed variables");

  absl::flat_hash_map<std::string, int64_t> dependent_variables;

  for (absl::string_view name : needed_dependent_variables) {
    MORIARTY_ASSIGN_OR_RETURN(
        dependent_variables[name], GetKnownValue<MInteger>(name),
        _ << "Error getting the dependent variable " << name);
  }

  MORIARTY_ASSIGN_OR_RETURN(std::optional<Range::ExtremeValues> extremes,
                            bounds_.Extremes(dependent_variables));
  if (!extremes) return absl::InvalidArgumentError("Valid range is empty");
  return *extremes;
}

MInteger& MInteger::WithSize(CommonSize size) {
  return AddConstraint(SizeCategory(size));
}

absl::Status MInteger::OfSizeProperty(Property property) {
  if (property.category != "size") {
    return absl::InvalidArgumentError(
        "Property's category must be 'size' in OfSizeProperty()");
  }

  CommonSize size = librarian::CommonSizeFromString(property.descriptor);
  if (size == CommonSize::kUnknown) {
    return absl::InvalidArgumentError(
        absl::Substitute("Unknown size: $0", property.descriptor));
  }

  WithSize(size);
  return absl::OkStatus();
}

absl::StatusOr<int64_t> MInteger::GenerateImpl() {
  MORIARTY_ASSIGN_OR_RETURN(
      Range::ExtremeValues extremes, GetExtremeValues(),
      _ << "Error while getting the min/max of the range in MInteger");

  if (approx_size_ == CommonSize::kAny) return GenerateInRange(extremes);

  // TODO(darcybest): Make this work for larger ranges.
  if ((extremes.min <= std::numeric_limits<int64_t>::min() / 2) &&
      (extremes.max >= std::numeric_limits<int64_t>::max() / 2)) {
    absl::StatusOr<int64_t> value = GenerateInRange(extremes);
    if (value.ok()) return value;
  }

  // Note: `max - min + 1` does not overflow because of the check above.
  Range range =
      librarian::GetRange(approx_size_, extremes.max - extremes.min + 1);

  MORIARTY_ASSIGN_OR_RETURN(std::optional<Range::ExtremeValues> rng_extremes,
                            range.Extremes());

  // If a special size has been requested, attempt to generate that. If that
  // fails, generate the full range.
  if (rng_extremes) {
    // Offset the values appropriately. These ranges were supposed to be for
    // [1, N].
    rng_extremes->min += extremes.min - 1;
    rng_extremes->max += extremes.min - 1;

    absl::StatusOr<int64_t> value = GenerateInRange(*rng_extremes);
    if (value.ok()) return value;
  }

  return GenerateInRange(extremes);
}

absl::StatusOr<int64_t> MInteger::GenerateInRange(
    Range::ExtremeValues extremes) {
  // moriarty::MInteger needs direct access its RandomEngine. All other
  // variables should call Random(MInteger().Between(x, y)) to get a random
  // integer.
  moriarty_internal::RandomEngine& rng =
      moriarty_internal::MVariableManager<MInteger, int64_t>(this)
          .GetRandomEngine();

  return rng.RandInt(extremes.min, extremes.max);
}

absl::Status MInteger::MergeFromImpl(const MInteger& other) {
  bounds_.Intersect(other.bounds_);

  std::optional<CommonSize> merged_size =
      librarian::MergeSizes(approx_size_, other.approx_size_);
  if (!merged_size.has_value()) {
    return absl::InvalidArgumentError(
        "Attempting to merge MIntegers with different size properties.");
  }
  WithSize(*merged_size);

  return absl::OkStatus();
}

absl::StatusOr<int64_t> MInteger::ReadImpl() {
  MORIARTY_ASSIGN_OR_RETURN(IOConfig * io_config, GetIOConfig());
  // TODO(darcybest): do parsing ourselves?
  MORIARTY_ASSIGN_OR_RETURN(std::string token, io_config->ReadToken());
  std::stringstream is(token);

  int64_t value;
  if (!(is >> value))
    return absl::InvalidArgumentError("Unable to read an integer.");
  std::string garbage;
  if (is >> garbage)
    return absl::InvalidArgumentError(
        "Found extra characters after reading an integer!");
  return value;
}

absl::Status MInteger::PrintImpl(const int64_t& value) {
  MORIARTY_ASSIGN_OR_RETURN(IOConfig * io_config, GetIOConfig());
  return io_config->PrintToken(std::to_string(value));
}

absl::Status MInteger::IsSatisfiedWithImpl(const int64_t& value) const {
  absl::StatusOr<Range::ExtremeValues> extremes = GetExtremeValues();
  MORIARTY_RETURN_IF_ERROR(
      CheckConstraint(extremes.status(), "range should be valid"));

  MORIARTY_RETURN_IF_ERROR(
      CheckConstraint(extremes->min <= value && value <= extremes->max,
                      absl::Substitute("$0 is not in the range [$1, $2]", value,
                                       extremes->min, extremes->max)));

  return absl::OkStatus();
}

absl::StatusOr<std::vector<MInteger>> MInteger::GetDifficultInstancesImpl()
    const {
  absl::StatusOr<Range::ExtremeValues> extremes = GetExtremeValues();

  int64_t min =
      extremes.ok() ? extremes->min : std::numeric_limits<int64_t>::min();
  int64_t max =
      extremes.ok() ? extremes->max : std::numeric_limits<int64_t>::max();

  // TODO(hivini): Create more interesting cases instead of a list of ints.
  std::vector<int64_t> values = {min};
  if (min != max) values.push_back(max);

  // Takes all elements in `insert_list` and inserts the ones between `min` and
  // `max` into `values`.

  auto insert_into_values = [&](absl::Span<const int64_t> insert_list) {
    for (int64_t v : insert_list) {
      // min and max are already in, only include values strictly in (min, max).
      if (min < v && v < max && absl::c_find(values, v) == values.end())
        values.push_back(v);
    }
  };

  // Small values
  insert_into_values({0, 1, 2, -1, -2});

  // Near powers of 2 (2^63 will be handled with min/max if applicable)
  for (int exp : {7, 8, 15, 16, 31, 32, 62}) {
    int64_t pow_two = 1LL << exp;
    insert_into_values({pow_two, pow_two + 1, pow_two - 1});
    insert_into_values({-pow_two, -pow_two + 1, -pow_two - 1});
  }

  // Relative to min/max
  insert_into_values({min / 2, max / 2, min + 1, max - 1});
  if (max >= 0) {
    int64_t square_root = std::sqrt(max);
    insert_into_values({square_root, square_root + 1, square_root - 1});
  }

  std::vector<MInteger> instances;
  instances.reserve(values.size());
  for (const auto& v : values) {
    instances.push_back(MInteger().Is(v));
  }

  return instances;
}

std::vector<std::string> MInteger::GetDependenciesImpl() const {
  absl::StatusOr<absl::flat_hash_set<std::string>> needed =
      bounds_.NeededVariables();
  if (!needed.ok()) return {};
  return std::vector<std::string>(needed->begin(), needed->end());
}

std::string MInteger::ToStringImpl() const {
  std::string result;
  if (approx_size_ != CommonSize::kAny)
    absl::StrAppend(&result, "size: ", librarian::ToString(approx_size_), "; ");
  absl::StrAppend(&result, "bounds: ", bounds_.ToString(), "; ");
  return result;
}

absl::StatusOr<std::string> MInteger::ValueToStringImpl(
    const int64_t& value) const {
  return std::to_string(value);
}

}  // namespace moriarty
