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

#include "src/variables/constraints/numeric_constraints.h"

#include <cstdint>
#include <limits>

#include "src/internal/range.h"

namespace moriarty {

Between::Between(int64_t minimum, int64_t maximum)
    : bounds_(minimum, maximum) {}

Between::Between(int64_t minimum, IntegerExpression maximum) {
  // Note: We are ignoring the error here since it will bubble up later when it
  // is used. We should consider alternatives.
  bounds_.AtLeast(minimum);
  bounds_.AtMost(maximum).IgnoreError();
}

Between::Between(IntegerExpression minimum, int64_t maximum) {
  // Note: We are ignoring the error here since it will bubble up later when it
  // is used. We should consider alternatives.
  bounds_.AtLeast(minimum).IgnoreError();
  bounds_.AtMost(maximum);
}

Between::Between(IntegerExpression minimum, IntegerExpression maximum) {
  // Note: We are ignoring the error here since it will bubble up later when it
  // is used. We should consider alternatives.
  bounds_.AtLeast(minimum).IgnoreError();
  bounds_.AtMost(maximum).IgnoreError();
}

Range Between::GetRange() const { return bounds_; }

AtMost::AtMost(int64_t maximum)
    : bounds_(std::numeric_limits<int64_t>::min(), maximum) {}

AtMost::AtMost(IntegerExpression maximum) {
  // Note: We are ignoring the error here since it will bubble up later when it
  // is used. We should consider alternatives.
  bounds_.AtMost(maximum).IgnoreError();
}

Range AtMost::GetRange() const { return bounds_; }

AtLeast::AtLeast(int64_t minimum)
    : bounds_(minimum, std::numeric_limits<int64_t>::max()) {}

AtLeast::AtLeast(IntegerExpression minimum) {
  // Note: We are ignoring the error here since it will bubble up later when it
  // is used. We should consider alternatives.
  bounds_.AtLeast(minimum).IgnoreError();
}

Range AtLeast::GetRange() const { return bounds_; }

}  // namespace moriarty
