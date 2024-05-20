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

#ifndef MORIARTY_SRC_INTERNAL_COMBINATORIAL_COVERAGE_H_
#define MORIARTY_SRC_INTERNAL_COMBINATORIAL_COVERAGE_H_

#include <functional>
#include <vector>

namespace moriarty {

struct CoveringArrayTestCase {
  std::vector<int> test_case;
};

// GenerateCoveringArray()
//
// Generates a covering array with prescribed `strength`.
//
// Each dimension has a certain size (the number of elements in it). The
// elements in a dimension are 0, 1, ... , size - 1.
//
// A covering array with strength t (1 <= t <= #dimensions) means that for *all*
// subsets of dimensions of size t, there is at one test case for all
// possibilities if you just consider the dimensions in that subset.
//
// For example, if dimension_sizes = {2, 3, 2}, then there are 12 possible test
// cases:
//   {(0, 0, 0), (0, 0, 1), (0, 1, 0), (0, 1, 1), (0, 2, 0), (0, 2, 1),
//    (1, 0, 0), (1, 0, 1), (1, 1, 0), (1, 1, 1), (1, 2, 0), (1, 2, 1) }
//
// However, consider these 8 test cases:
//    000 010 020 021 101 110 111 121
// Whichever pair of columns you choose, all possibilities appear. Thus, this
// set of 8 cases has strength 2.
//
// `rand` is a random function that takes in an integer n (n > 0) and should
// return a random integer x in the range 0 <= x < n. `rand` should be
// some form of uniformly-distributed random number generator (and not simply,
// for example, `return 0;`).
//
// On average, you should expect approximately this many elements in the output
// (this is not guaranteed):
//   O( t * D * log(N) )
// where:
//   t = strength
//   D = product of the t largest dimension sizes. Think: pow(max_dim_size, t).
//   N = number of dimensions
//
// NOTE: This implementation is currently limited to <= 62 dimensions.
std::vector<CoveringArrayTestCase> GenerateCoveringArray(
    std::vector<int> dimension_sizes, int strength,
    std::function<int(int)> rand);

}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_COMBINATORIAL_COVERAGE_H_
