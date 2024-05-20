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

#include "src/internal/range.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"
#include "src/internal/expressions.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

Range::Range(int64_t minimum, int64_t maximum) : min_(minimum), max_(maximum) {}

void Range::AtLeast(int64_t minimum) { min_ = std::max(min_, minimum); }

absl::Status Range::AtLeast(absl::string_view integer_expression) {
  absl::StatusOr<Expression> expr = ParseExpression(integer_expression);
  if (!expr.ok()) {
    absl::Status status = absl::InvalidArgumentError(absl::Substitute(
        "AtLeast called with invalid expression; $0", expr.status().message()));
    parameter_status_.Update(status);
    return status;
  }

  MORIARTY_ASSIGN_OR_RETURN(absl::flat_hash_set<std::string> vars,
                            moriarty::NeededVariables(*expr));
  needed_variables_.merge(vars);

  min_exprs_.push_back(std::move(*expr));
  return absl::OkStatus();
}

void Range::AtMost(int64_t maximum) { max_ = std::min(max_, maximum); }

absl::Status Range::AtMost(absl::string_view integer_expression) {
  absl::StatusOr<Expression> expr = ParseExpression(integer_expression);
  if (!expr.ok()) {
    absl::Status status = absl::InvalidArgumentError(absl::Substitute(
        "AtMost called with invalid expression; $0", expr.status().message()));
    parameter_status_.Update(status);
    return status;
  }

  MORIARTY_ASSIGN_OR_RETURN(absl::flat_hash_set<std::string> vars,
                            moriarty::NeededVariables(*expr));
  needed_variables_.merge(vars);

  max_exprs_.push_back(std::move(*expr));
  return absl::OkStatus();
}

namespace {

// Evaluate all expressions, then find the minimum/maximum, depending on the
// value of `compare`.
template <typename F>
absl::StatusOr<int64_t> FindExtreme(
    int64_t initial_value, absl::Span<const Expression> exprs,
    const absl::flat_hash_map<std::string, int64_t>& variables, F compare) {
  for (const Expression& expr : exprs) {
    MORIARTY_ASSIGN_OR_RETURN(int64_t val,
                              EvaluateIntegerExpression(expr, variables));
    if (compare(val, initial_value)) initial_value = val;
  }
  return initial_value;
}

}  // namespace

absl::StatusOr<std::optional<Range::ExtremeValues>> Range::Extremes(
    const absl::flat_hash_map<std::string, int64_t>& variables) const {
  MORIARTY_RETURN_IF_ERROR(parameter_status_);

  ExtremeValues extremes;
  MORIARTY_ASSIGN_OR_RETURN(
      extremes.min,
      FindExtreme(min_, min_exprs_, variables, std::greater<int64_t>()));
  MORIARTY_ASSIGN_OR_RETURN(
      extremes.max,
      FindExtreme(max_, max_exprs_, variables, std::less<int64_t>()));

  if (extremes.min > extremes.max) return std::nullopt;

  return extremes;
}

absl::StatusOr<absl::flat_hash_set<std::string>> Range::NeededVariables()
    const {
  MORIARTY_RETURN_IF_ERROR(parameter_status_);

  return needed_variables_;
}

void Range::Intersect(const Range& other) {
  parameter_status_.Update(other.parameter_status_);

  AtLeast(other.min_);
  AtMost(other.max_);

  min_exprs_.insert(min_exprs_.end(), other.min_exprs_.begin(),
                    other.min_exprs_.end());
  max_exprs_.insert(max_exprs_.end(), other.max_exprs_.begin(),
                    other.max_exprs_.end());

  needed_variables_.insert(other.needed_variables_.begin(),
                           other.needed_variables_.end());
}

namespace {

// Return a nice string representation of these bounds. If there is no
// restriction, then nullopt is returned. If there is one restriction, it will
// just return that. Otherwise, will return a comma separated list of bounds.
std::optional<std::string> BoundsToString(
    bool is_minimum, int64_t numeric_limit,
    absl::Span<const Expression> expression_limits) {
  bool unchanged_numeric_limit =
      (is_minimum && numeric_limit == std::numeric_limits<int64_t>::min()) ||
      (!is_minimum && numeric_limit == std::numeric_limits<int64_t>::max());

  // No restrictions.
  if (expression_limits.empty() && unchanged_numeric_limit) return std::nullopt;

  std::vector<std::string> bounds;
  bounds.reserve(expression_limits.size() + 1);
  if (!unchanged_numeric_limit) bounds.push_back(absl::StrCat(numeric_limit));
  for (const Expression& expr : expression_limits)
    bounds.push_back(expr.ToString());

  if (bounds.size() == 1) return bounds[0];

  // Note, we swap min/max here since we want max(a, b, c) <= x <= min(d, e, f).
  return absl::Substitute("$0($1)", (is_minimum ? "max" : "min"),
                          absl::StrJoin(bounds, ", "));
}

}  // namespace

std::string Range::ToString() const {
  if (min_ > max_) return "(Empty Range)";

  std::optional<std::string> min_bounds =
      BoundsToString(/* is_minimum = */ true, min_, min_exprs_);
  std::optional<std::string> max_bounds =
      BoundsToString(/* is_minimum = */ false, max_, max_exprs_);

  if (!min_bounds.has_value() && !max_bounds.has_value()) return "(-inf, inf)";
  if (!min_bounds.has_value())
    return absl::Substitute("(-inf, $0]", *max_bounds);
  if (!max_bounds.has_value())
    return absl::Substitute("[$0, inf)", *min_bounds);

  return absl::Substitute("[$0, $1]", *min_bounds, *max_bounds);
}

Range EmptyRange() { return Range(0, -1); }

}  // namespace moriarty
