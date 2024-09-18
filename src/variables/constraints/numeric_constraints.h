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

#ifndef MORIARTY_SRC_VARIABLES_NUMERIC_CONSTRAINTS_H_
#define MORIARTY_SRC_VARIABLES_NUMERIC_CONSTRAINTS_H_

#include <cstdint>

#include "absl/strings/string_view.h"
#include "src/internal/range.h"
#include "src/variables/constraints/base_constraints.h"

namespace moriarty {

using IntegerExpression = absl::string_view;

// Constraint stating that the numeric value must be in the inclusive range
// [minimum, maximum].
class Between : public MConstraint {
 public:
  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(int64_t minimum, int64_t maximum);

  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(IntegerExpression minimum, IntegerExpression maximum);

  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(int64_t minimum, IntegerExpression maximum);

  // The numeric value must be in the inclusive range [minimum, maximum].
  explicit Between(IntegerExpression minimum, int64_t maximum);

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

 private:
  Range bounds_;
};

// Constraint stating that the numeric value must be this value or smaller.
class AtMost : public MConstraint {
 public:
  // The numeric value must be this value or smaller. E.g., AtMost(123)
  explicit AtMost(int64_t maximum);

  // The numeric value must be this value or smaller.
  // E.g., AtMost("10^9") or AtMost("3 * N + 1")
  explicit AtMost(IntegerExpression maximum);

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

 private:
  Range bounds_;
};

// Constraint stating that the numeric value must be this value or larger.
class AtLeast : public MConstraint {
 public:
  // The numeric value must be this value or larger. E.g., AtLeast(123)
  explicit AtLeast(int64_t minimum);

  // The numeric value must be this value or larger.
  // E.g., AtLeast("10^9") or AtLeast("3 * N + 1")
  explicit AtLeast(IntegerExpression minimum);

  // Returns the range of values that this constraint represents.
  [[nodiscard]] Range GetRange() const;

 private:
  Range bounds_;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_NUMERIC_CONSTRAINTS_H_
