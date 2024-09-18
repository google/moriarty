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

// Size helper functions
//
// This file contains several helper functions for providing approximate sizes
// of ranges for subjective size categories.
//
// There are 7 main types of sizes:
//  * Extremal : min, max
//  * Primary  : small, medium, large
//  * Secondary: tiny, huge
//
// The ranges for each of these sizes may overlap with one another.

#ifndef MORIARTY_SRC_LIBRARIAN_SIZE_PROPERTY_H_
#define MORIARTY_SRC_LIBRARIAN_SIZE_PROPERTY_H_

#include <cstdint>
#include <optional>
#include <string>

#include "absl/strings/string_view.h"
#include "src/internal/range.h"

namespace moriarty {

enum class CommonSize {
  kAny,
  kMin,  // Start of "Extremal"
  kMax,
  kSmall,  // Start of "Primary"
  kMedium,
  kLarge,
  kTiny,  // Start of "Secondary"
  kHuge,

  // Additional values that are not part of the main enum. These should not be
  // considered as a size. (E.g., ToString(kUnknown) is not valid).
  kUnknown,
  kThisEnumIsNotAnExhaustiveListMoreItemsMayBeAddedInTheFuture
};

namespace librarian {

// MergeSizes()
//
// Returns the overlap between `size1` and `size2`.
//
// Relationships:
//
// * kAny is a superset of everything.
// * kMin is a subset of kTiny, which is a subset of kSmall.
// * kMax is a subset of kHuge, which is a subset of kLarge.
// * kMedium is not related to any other size.
//
// Returns `std::nullopt` if there is no overlap.
[[nodiscard]] std::optional<CommonSize> MergeSizes(CommonSize size1,
                                                   CommonSize size2);

// GetRange()
//
// Given a range of the form [1, N] and the approximate size, returns the
// appropriate range.
[[nodiscard]] moriarty::Range GetRange(CommonSize size, int64_t N);

// GetMinRange()
//
// Given a range of the form [1, N], returns the range for size = "min". For
// "min", this may be just a single value: the minimum value in the range.
[[nodiscard]] moriarty::Range GetMinRange(int64_t N);

// GetTinyRange()
//
// Given a range of the form [1, N], returns the range for size = "tiny".
// In general, "tiny" should be easy for a human to digest while debugging.
[[nodiscard]] moriarty::Range GetTinyRange(int64_t N);

// GetSmallRange()
//
// Given a range of the form [1, N], returns the range for size = "small".
[[nodiscard]] moriarty::Range GetSmallRange(int64_t N);

// GetMediumRange()
//
// Given a range of the form [1, N], returns the range for size = "medium".
[[nodiscard]] moriarty::Range GetMediumRange(int64_t N);

// GetLargeRange()
//
// Given a range of the form [1, N], returns the range for size = "large".
[[nodiscard]] moriarty::Range GetLargeRange(int64_t N);

// GetHugeRange()
//
// Given a range of the form [1, N], returns the range for size = "huge".
[[nodiscard]] moriarty::Range GetHugeRange(int64_t N);

// GetMaxRange()
//
// Given a range of the form [1, N], returns the range for size = "max". For
// "max", this may be just a single value: the maximum value in the range.
[[nodiscard]] moriarty::Range GetMaxRange(int64_t N);

// ToString()
//
// Returns a string representation of the size.
[[nodiscard]] std::string ToString(CommonSize size);

// CommonSizeFromString()
//
// Returns the CommonSize corresponding to the string. The string name for each
// size is lowercase version without the k. "CommonSize::kFoo" -> "foo".
//
// Returns kUnknown if the string is not a valid CommonSize.
[[nodiscard]] CommonSize CommonSizeFromString(absl::string_view size);

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_SIZE_PROPERTY_H_
