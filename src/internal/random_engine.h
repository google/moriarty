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

#ifndef MORIARTY_SRC_INTERNAL_RANDOM_ENGINE_H_
#define MORIARTY_SRC_INTERNAL_RANDOM_ENGINE_H_

// Moriarty's Internal Random implementation
//
// Only provides signed 64-bit integers. If the user would like unsigned, they
// will need a wrapper class around this.
//
// There are a plethora of random libraries out there. However, for Moriarty, we
// need to ensure consistency and portability. Users on different OS's and
// different compilers need to receive the same random numbers.
//
// C++'s STL functions are a mix of implementation-defined and well-defined
// functions. The largest issue are the *_distribution functions, which are
// implementation-defined (e.g., uniform_int_distribution). We re-implement
// these functions here to ensure portability.
//
// Under the hood, moriarty::Random uses std::mt19937_64. Note that this is
// well-defined, and we seed using std::seed_seq, which is also well-defined.
//
// However, note that the internal state of std::mt19937_64 is
// implementation-dependent, so do not use `RandomEngine() << s` or
// `RandomEngine() >> s` in hopes of saving the internal state for later. We
// provide `SaveState()` and `LoadState(state)` to re-load the state. Note that
// loading the state requires us to call the random generator many, many
// times, so it is not necessarily fast.

#include <cstdint>
#include <random>
#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

namespace moriarty {
namespace moriarty_internal {

class RandomEngine {
 public:
  // Constructors. Note that this constructor contains abnormal parameters.
  //
  // `moriarty_version_num` will be used to ensure backwards compatibility. At
  // the moment, this string is ignored.
  // TODO(b/182810006): Enforce a specific version_num for first launch.
  //
  // TODO(b/182810006): Our random seeds may be fixed length eventually.
  // Consider changing to std::array if that's the case.
  RandomEngine(absl::Span<const int64_t> seed,
               absl::string_view moriarty_version_num);

  // RandInt()
  //
  // Generates a uniformly random integer in the range:
  // [0, exclusive_upper_bound).
  //
  // Returns kInvalidArgument if exclusive_upper_bound is non-positive.
  absl::StatusOr<int64_t> RandInt(int64_t exclusive_upper_bound);

  // RandInt()
  //
  // Generates a uniformly random integer in the range:
  // [inclusive_lower_bound, inclusive_upper_bound].
  absl::StatusOr<int64_t> RandInt(int64_t inclusive_lower_bound,
                                  int64_t inclusive_upper_bound);

 private:
  // InitRandomEngine()
  //
  // Sets the internal state of the random engine appropriately. Separate from
  // the constructor so that we can later add SaveState and LoadState.
  void InitRandomEngine(
      absl::Span<const int64_t> seed,
      int64_t initial_discards = kInitialDiscardsFromRandomEngine);

  // RandomIntInclusive()
  //
  // Generates a uniformly random integer in the range:
  // [0,inclusive_upper_bound]
  uint64_t RandIntInclusive(uint64_t inclusive_upper_bound);

  // RandUInt64()
  //
  // Generates a random integer in the range [0, 2^64).
  uint64_t RandUInt64();

  // The random engine needs to be "warmed up" before producing truly random
  // values that are independent (in some sense) from the seeds passed in. This
  // acts as the "warm up". This fully clears Mersenne Twister's internal state
  // once and a bit.
  static constexpr int64_t kInitialDiscardsFromRandomEngine = 1024;
  const std::string moriarty_version_num_;
  std::mt19937_64 re_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_RANDOM_ENGINE_H_
