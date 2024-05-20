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

#include <functional>
#include <ostream>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/str_join.h"
#include "src/internal/combinatorial_coverage_test_util.h"

namespace moriarty {

// Print `CoveringArrayTestCase`s nicely for tests.
std::ostream& operator<<(std::ostream& os, const CoveringArrayTestCase& tc) {
  os << "[" << absl::StrJoin(tc.test_case, ", ") << "]";
  return os;
}

namespace {

using testing::SizeIs;

// For use in `GenerateCoveringArray`. No test should depend on the
// implementation of this.
std::function<int(int)> RandFn() {
  return [x = 0](int n) mutable -> int {
    x++;
    return x % n;
  };
}

TEST(CombinatorialCoverageTest,
     CoveringArrayWithFullStrengthShouldCoverEveryOption) {
  EXPECT_THAT(GenerateCoveringArray({3, 3, 3}, 3, RandFn()), SizeIs(27));
  EXPECT_THAT(GenerateCoveringArray({2, 3, 4, 2}, 4, RandFn()), SizeIs(48));
}

TEST(CombinatorialCoverageTest, ProducesACoveringArray) {
  EXPECT_THAT(GenerateCoveringArray({3, 3, 3, 3}, 2, RandFn()),
              IsStrength2CoveringArray(std::vector<int>({3, 3, 3, 3})));
  EXPECT_THAT(GenerateCoveringArray({3, 4, 1, 2}, 2, RandFn()),
              IsStrength2CoveringArray(std::vector<int>({3, 4, 1, 2})));
}

}  // namespace
}  // namespace moriarty
