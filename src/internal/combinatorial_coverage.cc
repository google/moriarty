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

#include "src/internal/combinatorial_coverage.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/log/absl_check.h"
#include "absl/types/span.h"

namespace moriarty {

namespace {

// We use bitmasks to reduce memory overhead. This limits the size to <= 62. If
// more dimensions are needed, we may wish to have a non-bit version since its
// likely the strength is small in that case.
using BitMask = uint64_t;
constexpr uint64_t ONE = 1;  // To avoid `(uint64_t)1 << val`

using SerializedTestCase = int64_t;

// Represents a subset of the columns and which (partial) test cases we have
// covered in those columns. The `covered` array stores which of the
//
//    `dim_1_size * dim_2_size * ... * dim_strength_size`
//
// partial test cases for this `column_mask` have been seen. The `covered` array
// corresponds to the test cases in lexicographic order.
struct ColumnSet {
  BitMask column_mask;          // Which columns?
  int64_t remaining_uncovered;  // How many partial cases are not covered?
  std::vector<bool> covered;    // Some case covers the serialized partial case.
};

// SerializePartialTestCase()
//
// Compresses the projection of a test case into a single integer. The bits set
// in `mask` are the only dimensions considered (projection onto these
// dimensions). For the projection, compresses the values into a single integer,
// the lexicographic order of this test case.
//
//  0 = (0, 0, 0, 0)
//  1 = (0, 0, 0, 1)
//  2 = (0, 0, 0, 2)
//  3 = (0, 0, 1, 0)
//   ...
//
// Assumes 0 <= test_cases[i] < dimension_sizes[i].
SerializedTestCase SerializePartialTestCase(
    BitMask mask, const CoveringArrayTestCase& test_case,
    absl::Span<const int> dimension_sizes) {
  SerializedTestCase result = 0;
  for (int i = 0; i < dimension_sizes.size(); i++) {
    if ((mask >> i) & ONE)
      result = test_case.test_case[i] + result * dimension_sizes[i];
  }
  return result;
}

// DeserializePartialTestCase()
//
// Opposite of SerializePartialTestCase(). Takes the serialized value and
// deserializes the appropriate fields into `partial_test_case`. All other
// fields are ignored.
void DeserializePartialTestCaseInto(BitMask mask,
                                    SerializedTestCase serialized_value,
                                    absl::Span<const int> dimension_sizes,
                                    CoveringArrayTestCase& partial_test_case) {
  for (int i = (int)dimension_sizes.size() - 1; i >= 0; i--) {
    if (!((mask >> i) & ONE)) continue;
    partial_test_case.test_case[i] = serialized_value % dimension_sizes[i];
    serialized_value /= dimension_sizes[i];
  }
  ABSL_CHECK_EQ(serialized_value, 0);
}

// GetAllNChooseK()
//
// Returns a list of all bitmasks of with n bits and k 1s.
std::vector<BitMask> GetAllNChooseK(int n, int k) {
  std::vector<BitMask> masks;

  // Magic bit-hack. Requires #bits <= 62.
  BitMask mask = (ONE << k) - 1;
  while (mask < (ONE << n)) {
    masks.push_back(mask);
    BitMask rightmost_bit = mask & (-mask);
    BitMask upper = mask + rightmost_bit;
    mask = upper | ((mask ^ upper) / rightmost_bit) >> 2;
  }
  return masks;
}

// Find some test case to add.
// (1) This function greedily finds the set of columns that has the most
//     uncovered partial test cases, then randomly selects one of those
//     uncovered partial test cases.
// (2) Greedily find the set of columns that do not conflict with the already
//     chosen columns and randomly selects one of those uncovered partial test
//     cases.
// (3) Repeat (2) until all columns have been covered.
std::optional<CoveringArrayTestCase> FindTestCaseToAdd(
    absl::Span<const ColumnSet> column_sets,
    absl::Span<const int> dimension_sizes, std::function<int(int)> rand) {
  // Sort column_sets by number of remaining uncovered partial test cases. We
  // sort the indices rather than the columns themselves.
  std::vector<int64_t> column_set_index(column_sets.size());
  absl::c_iota(column_set_index, 0);
  // Using stable_sort for consistency across systems.
  absl::c_stable_sort(column_set_index,
                      [column_sets](int64_t idx1, int64_t idx2) {
                        return column_sets[idx1].remaining_uncovered >
                               column_sets[idx2].remaining_uncovered;
                      });

  // If there is nothing left to cover, we are done!
  if (column_sets[column_set_index[0]].remaining_uncovered == 0)
    return std::nullopt;

  // `used` is the set of columns do we already have a value for.
  BitMask used_mask = 0;

  int n = dimension_sizes.size();
  CoveringArrayTestCase result_test_case({.test_case = std::vector<int>(n)});
  for (int i = 0; i < column_set_index.size(); i++) {
    const ColumnSet& column_set = column_sets[column_set_index[i]];
    if (used_mask & column_set.column_mask) continue;
    if (column_set.remaining_uncovered == 0) continue;

    // Choose a random value that hasn't been seen before.
    int64_t projected_value;
    while (true) {
      projected_value = rand(column_set.covered.size());
      if (!column_set.covered[projected_value]) break;
    }

    DeserializePartialTestCaseInto(column_set.column_mask, projected_value,
                                   dimension_sizes, result_test_case);
    used_mask |= column_set.column_mask;
  }

  // At this point, some fields may not be set still. Randomly assign those now.
  for (int i = 0; i < n; i++) {
    if (!((used_mask >> i) & ONE))
      result_test_case.test_case[i] = rand(dimension_sizes[i]);
  }
  return result_test_case;
}

// Updates `column_set` to signify that `test_case` has been added to the set of
// test cases.
void UpdateColumnSet(const CoveringArrayTestCase& test_case,
                     absl::Span<const int> dimension_sizes,
                     ColumnSet& column_set) {
  SerializedTestCase serial_test_case = SerializePartialTestCase(
      column_set.column_mask, test_case, dimension_sizes);

  if (column_set.covered[serial_test_case]) return;  // Already covered.
  column_set.covered[serial_test_case] = true;
  column_set.remaining_uncovered--;
}

// Create all (n choose strength) column sets. Each set of columns will have its
// `covered` array set to the appropriate number of values.
std::vector<ColumnSet> InitializeColumnSets(
    int n, int strength, absl::Span<const int> dimension_sizes) {
  std::vector<BitMask> masks = GetAllNChooseK(n, strength);
  std::vector<ColumnSet> column_sets;
  column_sets.reserve(masks.size());

  for (BitMask mask : masks) {
    int64_t partial_test_case_size = 1;
    for (int i = 0; i < dimension_sizes.size(); i++)
      if ((mask >> i) & ONE) partial_test_case_size *= dimension_sizes[i];

    column_sets.push_back(ColumnSet(
        {.column_mask = mask,
         .remaining_uncovered = partial_test_case_size,
         .covered = std::vector<bool>(partial_test_case_size, false)}));
  }
  return column_sets;
}

}  // namespace

std::vector<CoveringArrayTestCase> GenerateCoveringArray(
    std::vector<int> dimension_sizes, int strength,
    std::function<int(int)> rand) {
  ABSL_CHECK_GT(strength, 0) << "Strength must be > 0";
  ABSL_CHECK_LE(strength, dimension_sizes.size())
      << "Strength must be <= #dims";

  for (int size : dimension_sizes) {
    ABSL_CHECK_GT(size, 0) << "Dimension sizes must be > 0";
  }
  ABSL_CHECK_LE(dimension_sizes.size(), 62)
      << "Number of dimensions must be <= 62 for the bitmasks to work.";

  int n = dimension_sizes.size();
  std::vector<ColumnSet> column_sets =
      InitializeColumnSets(n, strength, dimension_sizes);

  std::vector<CoveringArrayTestCase> result;
  while (true) {
    std::optional<CoveringArrayTestCase> test_case =
        FindTestCaseToAdd(column_sets, dimension_sizes, rand);

    if (test_case == std::nullopt) break;

    result.push_back(*test_case);
    for (ColumnSet& column_set : column_sets)
      UpdateColumnSet(*test_case, dimension_sizes, column_set);
  }

  return result;
}

}  // namespace moriarty
