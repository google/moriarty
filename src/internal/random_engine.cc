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

#include "src/internal/random_engine.h"

#include <cstdint>
#include <limits>
#include <random>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/span.h"

namespace moriarty {
namespace moriarty_internal {

RandomEngine::RandomEngine(absl::Span<const int64_t> seed,
                           absl::string_view moriarty_version_num)
    : moriarty_version_num_(moriarty_version_num) {
  InitRandomEngine(seed);
}

absl::StatusOr<int64_t> RandomEngine::RandInt(int64_t exclusive_upper_bound) {
  if (exclusive_upper_bound <= 0) {
    return absl::InvalidArgumentError(absl::Substitute(
        "RandInt(x) called with x <= 0 ($0)", exclusive_upper_bound));
  }
  return RandIntInclusive(exclusive_upper_bound - 1);
}

absl::StatusOr<int64_t> RandomEngine::RandInt(int64_t inclusive_lower_bound,
                                              int64_t inclusive_upper_bound) {
  if (inclusive_lower_bound > inclusive_upper_bound) {
    return absl::InvalidArgumentError(
        absl::Substitute("RandInt(x, y) called with x > y ($0, $1)",
                         inclusive_lower_bound, inclusive_upper_bound));
  }
  // This static_cast from signed to unsigned was implementation-defined
  // behavior prior to C++20. Now should be okay, I think.
  return inclusive_lower_bound +
         RandIntInclusive(static_cast<uint64_t>(inclusive_upper_bound) -
                          static_cast<uint64_t>(inclusive_lower_bound));
}

void RandomEngine::InitRandomEngine(absl::Span<const int64_t> seed,
                                    int64_t initial_discards) {
  std::seed_seq sseq(std::begin(seed), std::end(seed));
  re_.seed(sseq);
  re_.discard(initial_discards);
}

uint64_t RandomEngine::RandIntInclusive(uint64_t inclusive_upper_bound) {
  const uint64_t range = inclusive_upper_bound + 1;
  if ((inclusive_upper_bound & range) == 0) {
    // Interval's length is a power of two, just take the low bits. In
    // particular, if range == 2^64 (== 0), this just returns RandUInt64();
    return RandUInt64() & inclusive_upper_bound;
  }

  // Create acceptance ranges of size `scale`:
  //
  //   [0, scale) -> 0
  //   [scale, 2 * scale) -> 1
  //      ...
  //   [i * scale, (i+1) * scale) -> i
  //      ...
  //   [(range-1) * scale, range * scale) -> range-1 (= inclusive_upper_bound)
  //
  // If we're outside this area, reject and try again. This will happen with
  // probability < 1/2, and almost surely much less than 1/2.
  //
  // [range * scale, oo) -> reject
  //
  // Note: This assumes range != 2^64, since re_.max() == 2^64-1. However, if
  // range == 2^64, then the power of two check above would catch this.
  uint64_t scale = std::numeric_limits<uint64_t>::max() / range;
  uint64_t limit = range * scale;

  uint64_t answer;
  do {
    answer = RandUInt64();
  } while (answer >= limit);

  return answer / scale;
}

uint64_t RandomEngine::RandUInt64() { return re_(); }

}  // namespace moriarty_internal
}  // namespace moriarty
