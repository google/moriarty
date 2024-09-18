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

#ifndef MORIARTY_SRC_VARIABLES_CONSTRAINTS_SIZE_CONSTRAINTS_H_
#define MORIARTY_SRC_VARIABLES_CONSTRAINTS_SIZE_CONSTRAINTS_H_

#include "absl/strings/string_view.h"
#include "src/librarian/size_property.h"
#include "src/variables/constraints/base_constraints.h"

namespace moriarty {

// Weak constraint stating that the value must have this approximate size. These
// sizes are not well-defined and may change over time. In general:
//
//     min <= tiny <= small <= medium <= large <= huge <= max.
//
// There are overlaps in values between these sizes. For example, a value
// could be considered both small and tiny.
//
// This constraint is intended to be used as a generation hint, not as a
// validation function. That is, an MVariable should not validate that it is
// exactly the size it was asked to be. Instead, it should generate a value of
// the appropriate size.
class SizeCategory : public MConstraint {
 public:
  // The value must be approximately this size.
  explicit SizeCategory(CommonSize size);

  // The value must be approximately this size.
  // Valid values are:
  //   "any", "min", "tiny", "small", "medium", "large", "huge", "max"
  explicit SizeCategory(absl::string_view size);

  // The value has no constraints on size.
  static SizeCategory Any();
  // The value should be the minimum size.
  static SizeCategory Min();
  // The value should be a tiny size.
  static SizeCategory Tiny();
  // The value should be a small size.
  static SizeCategory Small();
  // The value should be a medium size.
  static SizeCategory Medium();
  // The value should be a large size.
  static SizeCategory Large();
  // The value should be a huge size.
  static SizeCategory Huge();
  // The value should be the maximum size.
  static SizeCategory Max();

  // Returns the underlying size.
  [[nodiscard]] CommonSize GetCommonSize() const;

 private:
  CommonSize size_;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CONSTRAINTS_SIZE_CONSTRAINTS_H_
