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

#include "src/internal/random_config.h"

#include <stdint.h>

#include <concepts>
#include <vector>

#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "src/internal/random_engine.h"
#include "src/testing/random_test_util.h"

namespace moriarty_testing {
namespace {

class SeededRandomTests {
 public:
  SeededRandomTests() : engine_({1, 2, 3}, "v0.1") {}

  absl::StatusOr<int64_t> RandomInteger(int64_t min, int64_t max) {
    return moriarty::moriarty_internal::RandomInteger(engine_, min, max);
  }

  absl::StatusOr<int64_t> RandomInteger(int64_t n) {
    return moriarty::moriarty_internal::RandomInteger(engine_, n);
  }

  template <typename T>
  absl::Status Shuffle(std::vector<T>& container) {
    return moriarty::moriarty_internal::Shuffle(engine_, container);
  }

  template <typename T>
  absl::StatusOr<T> RandomElement(const std::vector<T>& container) {
    return moriarty::moriarty_internal::RandomElement(engine_, container);
  }

  template <typename T>
  absl::StatusOr<std::vector<T>> RandomElementsWithReplacement(
      const std::vector<T>& container, int k) {
    return moriarty::moriarty_internal::RandomElementsWithReplacement(
        engine_, container, k);
  }

  template <typename T>
  absl::StatusOr<std::vector<T>> RandomElementsWithoutReplacement(
      const std::vector<T>& container, int k) {
    return moriarty::moriarty_internal::RandomElementsWithoutReplacement(
        engine_, container, k);
  }

  absl::StatusOr<std::vector<int>> RandomPermutation(int n) {
    return moriarty::moriarty_internal::RandomPermutation(engine_, n);
  }

  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> RandomPermutation(int n, T min) {
    return moriarty::moriarty_internal::RandomPermutation(engine_, n, min);
  }

  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> DistinctIntegers(T n, int k, T min = 0) {
    return moriarty::moriarty_internal::DistinctIntegers(engine_, n, k, min);
  }

  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> RandomComposition(T n, int k,
                                                   T min_bucket_size = 1) {
    return moriarty::moriarty_internal::RandomComposition(engine_, n, k,
                                                          min_bucket_size);
  }

 private:
  moriarty::moriarty_internal::RandomEngine engine_;
};

// Most of the API is tested via this test suite.
INSTANTIATE_TYPED_TEST_SUITE_P(RandomConfig, ValidInputRandomnessTests,
                               SeededRandomTests);
INSTANTIATE_TYPED_TEST_SUITE_P(RandomConfig, InvalidInputRandomnessTests,
                               SeededRandomTests);

}  // namespace
}  // namespace moriarty_testing

namespace moriarty {
namespace moriarty_internal {
namespace {

TEST(RandomConfigTest, GetRandomEngineShouldReturnNullIfNoEngineSet) {
  RandomConfig config;
  EXPECT_EQ(config.GetRandomEngine(), nullptr);
}

TEST(RandomConfigTest, GetRandomEngineShouldReturnTheSetRandomEngine) {
  RandomEngine engine({1, 2, 3}, "v0.1");
  RandomConfig config;
  config.SetRandomEngine(&engine);

  EXPECT_EQ(config.GetRandomEngine(), &engine);
}

/*
TEST(RandomConfigTest, AllFunctionsFailWithoutRandomEngineSet) {
  RandomConfig config;
  // To use on functions that require a container.
  std::vector<int> helper = {1, 2, 3};

  EXPECT_THAT(config.Shuffle(helper),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(config.RandomElement(helper),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(config.RandomElementsWithoutReplacement(helper, 2),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(config.RandomElementsWithReplacement(helper, 2),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(config.RandomPermutation(5),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(config.RandomPermutation(5, 100),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(config.DistinctIntegers(10, 5),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
  EXPECT_THAT(config.RandomComposition(10, 3),
              IsMisconfigured(InternalConfigurationType::kRandomEngine));
}
*/

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
