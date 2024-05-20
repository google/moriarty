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

#ifndef MORIARTY_SRC_INTERNAL_COMBINATORIAL_COVERAGE_TEST_UTIL_H_
#define MORIARTY_SRC_INTERNAL_COMBINATORIAL_COVERAGE_TEST_UTIL_H_

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/absl_log.h"
#include "src/internal/combinatorial_coverage.h"

namespace moriarty {
// Checks if `arg` is a covering array with strength exactly 2.
MATCHER_P(IsStrength2CoveringArray, dimension_sizes,
          "is a covering array of strength 2") {
  if (arg.empty()) return testing::AssertionFailure() << "Empty array";

  int n = dimension_sizes.size();
  for (const CoveringArrayTestCase& tc : arg) {
    ABSL_LOG(INFO) << tc.test_case.size() << " " << n;
    if (tc.test_case.size() != n) {
      return testing::AssertionFailure()
             << "One of the test cases is the wrong size";
    }
  }

  for (const CoveringArrayTestCase& tc : arg) {
    for (int i = 0; i < n; i++) {
      if (!(0 <= tc.test_case[i] && tc.test_case[i] < dimension_sizes[i])) {
        return testing::AssertionFailure()
               << "One of the dimensions is out of bounds.";
      }
    }
  }

  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      for (int val_i = 0; val_i < dimension_sizes[i]; val_i++) {
        for (int val_j = 0; val_j < dimension_sizes[j]; val_j++) {
          bool found = false;
          for (int k = 0; k < arg.size(); k++) {
            if (arg[k].test_case[i] == val_i && arg[k].test_case[j] == val_j) {
              found = true;
              break;
            }
          }
          if (!found)
            return testing::AssertionFailure()
                   << "Cannot find " << val_i << ", " << val_j << " in columns "
                   << i << " " << j;
        }
      }
    }
  }
  return true;
}
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_COMBINATORIAL_COVERAGE_TEST_UTIL_H_
