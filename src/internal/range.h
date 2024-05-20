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

#ifndef MORIARTY_SRC_INTERNAL_RANGE_H_
#define MORIARTY_SRC_INTERNAL_RANGE_H_

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/internal/expressions.h"

namespace moriarty {

// Range
//
// All integers between min and max, inclusive.
//
// * An empty range can be created by setting min > max.
// * Additional calls to `AtMost` and `AtLeast` add extra constraints, and does
//   not overwrite the old ones.
class Range {
 public:
  // Creates a range covering all 64-bit signed integers.
  Range() = default;

  // Creates a range covering [`minimum`, `maximum`].
  // If `minimum` > `maximum`, then the range is empty.
  Range(int64_t minimum, int64_t maximum);

  // AtLeast()
  //
  // This range is at least `minimum`. For example,
  //   `AtLeast(5)`
  //
  // Multiple calls to `AtLeast` are ANDed  together. For example,
  //   `AtLeast(5); AtLeast("X + Y"); AtLeast("W");`
  // means that this is at least max({5, Evaluate("X + Y"), Evaluate("W")}).
  void AtLeast(int64_t minimum);

  // AtLeast()
  //
  // This range is at least `integer_expression`. For example,
  //   `AtLeast("3 * N + 1")`
  //
  // Multiple calls to `AtLeast` are ANDed  together. For example,
  //   `AtLeast(5); AtLeast("X + Y"); AtLeast("W");`
  // means that this is at least max({5, Evaluate("X + Y"), Evaluate("W")}).
  //
  // Returns a non-OK status if parsing fails.
  absl::Status AtLeast(absl::string_view integer_expression);

  // AtMost()
  //
  // This range is at least `maximum`. For example,
  //   `AtMost(5)`
  //
  // Multiple calls to `AtMost` are ANDed  together. For example,
  //   `AtMost(5); AtMost("X + Y"); AtMost("W");`
  // means that this is at least min({5, Evaluate("X + Y"), Evaluate("W")}).
  void AtMost(int64_t maximum);

  // AtMost()
  //
  // This range is at most `integer_expression`. For example,
  //   `AtMost("3 * N + 1")`
  //
  // Multiple calls to `AtMost` are ANDed  together. For example,
  //   `AtMost(5); AtMost("X + Y"); AtMost("W");`
  // means that this is at most min({5, Evaluate("X + Y"), Evaluate("W")}).
  //
  // Returns a non-OK status if parsing fails.
  absl::Status AtMost(absl::string_view integer_expression);

  struct ExtremeValues {
    int64_t min;
    int64_t max;
  };

  // Extremes()
  //
  // Returns the two extremes of the range (min and max). Returns `std::nullopt`
  // if the range is empty.
  absl::StatusOr<std::optional<ExtremeValues>> Extremes(
      const absl::flat_hash_map<std::string, int64_t>& variables = {}) const;

  // NeededVariables()
  //
  // Returns a set of all variable's values needed in order to evaluate
  // `Extremes()`.
  absl::StatusOr<absl::flat_hash_set<std::string>> NeededVariables() const;

  // Intersect()
  //
  // Intersects `other` with this Range (updating this range with the
  // intersection).
  void Intersect(const Range& other);

  // ToString()
  //
  // Returns a string representation of this range.
  [[nodiscard]] std::string ToString() const;

  // Determine if two ExtremeValues are equal. Used mainly for testing.
  friend bool operator==(const Range::ExtremeValues& e1,
                         const Range::ExtremeValues& e2) {
    return std::tie(e1.min, e1.max) == std::tie(e2.min, e2.max);
  }

 private:
  int64_t min_ = std::numeric_limits<int64_t>::min();
  int64_t max_ = std::numeric_limits<int64_t>::max();

  // If any input argument was invalid, then this stores that for when it is
  // actually needed.
  //
  // TODO(darcybest): This should not be lazily stored long term. We should pass
  // the value back to the user and they should fail as needed.
  absl::Status parameter_status_ = absl::OkStatus();

  // `min_exprs_` and `max_exprs_` are lists of Expressions that represent the
  // lower/upper bounds. They must be evaluated when `Extremes()` is called in
  // order to determine which is largest/smallest.
  std::vector<Expression> min_exprs_;
  std::vector<Expression> max_exprs_;

  absl::flat_hash_set<std::string> needed_variables_;
};

// Creates a range with no elements in it.
Range EmptyRange();

}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_RANGE_H_
