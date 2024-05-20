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

#ifndef MORIARTY_SRC_INTERNAL_RANDOM_CONFIG_H_
#define MORIARTY_SRC_INTERNAL_RANDOM_CONFIG_H_

#include <stddef.h>

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/substitute.h"
#include "src/internal/random_engine.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {
namespace moriarty_internal {

// Provides common Random functionality.
class RandomConfig {
 public:
  // Sets the random engine.
  RandomConfig& SetRandomEngine(RandomEngine* engine);

  // Returns `nullptr` if no RandomEngine has been set.
  RandomEngine* GetRandomEngine();

 private:
  RandomEngine* engine_ = nullptr;
};

// RandomInteger()
//
// Returns a random integer in the closed interval [min, max].
absl::StatusOr<int64_t> RandomInteger(RandomEngine& engine, int64_t min,
                                      int64_t max);

// RandomInteger()
//
// Returns a random integer in the semi-closed interval [0, n). Useful for
// random indices.
absl::StatusOr<int64_t> RandomInteger(RandomEngine& engine, int64_t n);

// Shuffle()
//
// Shuffles the elements in `container`.
template <typename T>
absl::Status Shuffle(RandomEngine& engine, std::vector<T>& container);

// RandomElement()
//
// Returns a random element of `container`.
template <typename T>
absl::StatusOr<T> RandomElement(RandomEngine& engine,
                                const std::vector<T>& container);

// RandomElementsWithReplacement()
//
// Returns k (randomly ordered) elements of `container`, possibly with
// duplicates.
template <typename T>
absl::StatusOr<std::vector<T>> RandomElementsWithReplacement(
    RandomEngine& engine, const std::vector<T>& container, int k);

// RandomElementsWithoutReplacement()
//
// Returns k (randomly ordered) elements of `container`, without duplicates.
//
// Each element may appear at most once. Note that if there are duplicates in
// `container`, each of those could be returned once each.
//
// So RandomElementsWithoutReplacement({0, 1, 1, 1}, 2) could return {1, 1}.
template <typename T>
absl::StatusOr<std::vector<T>> RandomElementsWithoutReplacement(
    RandomEngine& engine, const std::vector<T>& container, int k);

// RandomPermutation()
//
// Returns a random permutation of {0, 1, ... , n-1}.
absl::StatusOr<std::vector<int>> RandomPermutation(RandomEngine& engine, int n);

// RandomPermutation()
//
// Returns a random permutation of {min, min + 1, ... , min + (n-1)}.
//
// Requires min + (n-1) to not overflow T.
template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> RandomPermutation(RandomEngine& engine, int n,
                                                 T min);

// DistinctIntegers()
//
// Returns k (randomly ordered) distinct integers from
// {min, min + 1, ... , min + (n-1)}.
//
// Requires min + (n-1) to not overflow T.
template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> DistinctIntegers(RandomEngine& engine, T n,
                                                int k, T min = 0);

// RandomComposition()
//
// Returns a random composition (a partition where the order of the buckets is
// important) with k buckets. Each bucket has at least `min_bucket_size`
// elements and the sum of the `k` bucket sizes is `n`.
//
// The returned values are the sizes of each bucket. Note that (1, 2) is
// different from (2, 1).
//
// Requires n + (k - 1) to not overflow T.
template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> RandomComposition(RandomEngine& engine, T n,
                                                 int k, T min_bucket_size = 1);

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
absl::Status Shuffle(RandomEngine& engine, std::vector<T>& container) {
  for (size_t i = 1; i < container.size(); i++) {
    using std::swap;
    MORIARTY_ASSIGN_OR_RETURN(size_t swap_with, engine.RandInt(i + 1));
    if (i != swap_with) swap(container[i], container[swap_with]);
  }
  return absl::OkStatus();
}

template <typename T>
absl::StatusOr<T> RandomElement(RandomEngine& engine,
                                const std::vector<T>& container) {
  if (container.empty()) {
    return absl::InvalidArgumentError(
        "Cannot get a random element from an empty container");
  }

  MORIARTY_ASSIGN_OR_RETURN(int index, engine.RandInt(container.size()));
  return container[index];
}

template <typename T>
absl::StatusOr<std::vector<T>> RandomElementsWithReplacement(
    RandomEngine& engine, const std::vector<T>& container, int k) {
  if (k < 0) {
    return absl::InvalidArgumentError("k must be non-negative");
  }
  if (container.empty() && k > 0) {
    return absl::InvalidArgumentError(
        "Cannot get random elements from an empty container");
  }

  std::vector<T> result;
  result.reserve(k);

  for (int i = 0; i < k; i++) {
    MORIARTY_ASSIGN_OR_RETURN(int index, engine.RandInt(container.size()));
    result.push_back(container[index]);
  }
  return result;
}

template <typename T>
absl::StatusOr<std::vector<T>> RandomElementsWithoutReplacement(
    RandomEngine& engine, const std::vector<T>& container, int k) {
  if (k < 0) {
    return absl::InvalidArgumentError("k must be non-negative");
  }
  if (k > container.size()) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Cannot get $0 distinct element from a container of size $1", k,
        container.size()));
  }

  std::vector<T> result;
  result.reserve(k);

  MORIARTY_ASSIGN_OR_RETURN(std::vector<size_t> indices,
                            DistinctIntegers(engine, container.size(), k));
  for (size_t index : indices) result.push_back(container[index]);
  return result;
}

template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> RandomPermutation(RandomEngine& engine, int n,
                                                 T min) {
  if (n < 0) {
    return absl::InvalidArgumentError("n must be non-negative");
  }

  return DistinctIntegers(engine, T{n}, n, min);
}

template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> DistinctIntegers(RandomEngine& engine, T n,
                                                int k, T min) {
  if (n < 0) {
    return absl::InvalidArgumentError("n must be non-negative");
  }
  if (k < 0) {
    return absl::InvalidArgumentError("k must be non-negative");
  }
  if (k > n) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Cannot generate $0 distinct numbers from a range of size $1", k, n));
  }

  if (2 * k > n) {
    // If we are asking for a dense set, then we generate the whole search
    // space, shuffle it and return the first k elements. This will allocate at
    // most 2 times the required memory.
    std::vector<T> all;
    all.reserve(n);
    for (int i = 0; i < n; i++) all.push_back(i + min);
    MORIARTY_RETURN_IF_ERROR(Shuffle(engine, all));
    all.resize(k);
    return all;
  }

  // On average, the sampling should take fewer than log(2) * n iterations
  // since k <= n/2.
  absl::flat_hash_set<int64_t> sample;  // Stores the offsets {0, 1, ... , n-1}
  sample.reserve(k);

  // The order of `result` is the order we pull them from the RandomEngine (to
  // preserve order on different machines).
  std::vector<T> result;
  result.reserve(k);

  while (result.size() < k) {
    MORIARTY_ASSIGN_OR_RETURN(int64_t offset, engine.RandInt(n));
    auto [it, inserted] = sample.insert(offset);
    if (inserted) result.push_back(offset + min);
  }

  return result;
}

template <typename T>
  requires std::integral<T>
absl::StatusOr<std::vector<T>> RandomComposition(RandomEngine& engine, T n,
                                                 int k, T min_bucket_size) {
  if (n < 0) {
    return absl::InvalidArgumentError("n must be non-negative");
  }
  if (min_bucket_size < 0) {
    return absl::InvalidArgumentError("min_bucket_size must be non-negative");
  }
  if (n == 0 && k == 0 && min_bucket_size == 0) {
    return std::vector<T>();
  }
  if (k <= 0) {
    return absl::InvalidArgumentError(
        "k must be positive since one of n or min_bucket_size is");
  }
  if (min_bucket_size > 0 && n / min_bucket_size < k) {
    return absl::InvalidArgumentError(
        absl::Substitute("Impossible to put at least $0 entries into $1 "
                         "buckets when only $2 values possible",
                         min_bucket_size, k, n));
  }

  if (min_bucket_size > 0) {
    // Set the required size aside, generate a partition, then put them back.
    MORIARTY_ASSIGN_OR_RETURN(
        std::vector<T> result,
        RandomComposition(engine, n - min_bucket_size * k, k, 0));
    for (T& val : result) val += min_bucket_size;
    return result;
  }
  if (n == 0) return std::vector<T>(k, 0);

  // We are now placing (k-1) "barriers" amongst the n values, which is
  // equivalent to (n + (k-1)) choose (k-1).
  MORIARTY_ASSIGN_OR_RETURN(std::vector<T> barriers,
                            DistinctIntegers(engine, n + (k - 1), k - 1));
  std::sort(barriers.begin(), barriers.end());

  std::vector<T> result;
  result.reserve(k);

  T prev = -1;  // Add a barrier at index -1 to bookend the left side.
  for (T b : barriers) {
    result.push_back(b - prev - 1);
    prev = b;
  }
  result.push_back((n + (k - 1)) - prev - 1);

  return result;
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_RANDOM_CONFIG_H_
