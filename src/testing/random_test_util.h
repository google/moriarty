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

#ifndef MORIARTY_SRC_TESTING_RANDOM_TEST_UTIL_H_
#define MORIARTY_SRC_TESTING_RANDOM_TEST_UTIL_H_

#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty_testing {

// This set of tests validates the RandomConfig API. Several classes in Moriarty
// use the RandomConfig as part of their API (as a direct pass-through). Those
// classes should test using this typed test suite.
//
// This does not test any preconditions on the functions (such as seeding).
//
// All functions must return an absl::Status. If they don't, see
// `WrapRandomFuncsInStatus`. We separate valid and invalid function calls since
// the behavior may vary (either return a Status or crash). We only provide
// testing for the Status version here.
//
// Usage (put this in your *_test.cc file):
//
//   namespace moriarty_testing {
//   INSTANTIATE_TYPED_TEST_SUITE_P(
//         YourClassName, ValidInputRandomnessTests, YourClassName);
//   INSTANTIATE_TYPED_TEST_SUITE_P(
//         YourClassName, InvalidInputRandomnessTests, YourClassName);
//   }  // namespace moriarty_testing
//
// If your functions crash on failure, use this for the valid tests instead:
//
//   INSTANTIATE_TYPED_TEST_SUITE_P(
//         YourClassName, ValidInputRandomnessTests,
//         WrapRandomFuncsInStatus<YourClassName>);
//
// The tested class must have a default constructor and have the following
// functions whose signature exactly matches the API of `RandomConfig`. (You may
// need to create a test wrapper class in order to match the API exactly).
//
//  * RandomInteger()
//  * Shuffle()
//  * RandomElement()
//  * RandomElementsWithReplacement()
//  * RandomElementsWithoutReplacement()
//  * RandomPermutation()
//  * DistinctIntegers()
//  * RandomComposition()
template <typename T>
class ValidInputRandomnessTests : public testing::Test {};
template <typename T>
class InvalidInputRandomnessTests : public testing::Test {};

TYPED_TEST_SUITE_P(ValidInputRandomnessTests);
TYPED_TEST_SUITE_P(InvalidInputRandomnessTests);

// Wraps non-Status versions of the functions under test into a Status version.
template <typename TypeUnderTest>
class WrapRandomFuncsInStatus {
 public:
  absl::StatusOr<int64_t> RandomInteger(int64_t min, int64_t max) {
    return under_test_.RandomInteger(min, max);
  }

  absl::StatusOr<int64_t> RandomInteger(int64_t n) {
    return under_test_.RandomInteger(n);
  }

  template <typename T>
  absl::Status Shuffle(std::vector<T>& container) {
    under_test_.Shuffle(container);
    return absl::OkStatus();
  }

  template <typename T>
  absl::StatusOr<T> RandomElement(const std::vector<T>& container) {
    return under_test_.RandomElement(container);
  }

  template <typename T>
  absl::StatusOr<std::vector<T>> RandomElementsWithReplacement(
      const std::vector<T>& container, int k) {
    return under_test_.RandomElementsWithReplacement(container, k);
  }

  template <typename T>
  absl::StatusOr<std::vector<T>> RandomElementsWithoutReplacement(
      const std::vector<T>& container, int k) {
    return under_test_.RandomElementsWithoutReplacement(container, k);
  }

  absl::StatusOr<std::vector<int>> RandomPermutation(int n) {
    return under_test_.RandomPermutation(n);
  }

  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> RandomPermutation(int n, T min) {
    return under_test_.RandomPermutation(n, min);
  }

  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> DistinctIntegers(T n, int k, T min = 0) {
    return under_test_.DistinctIntegers(n, k, min);
  }

  template <typename T>
    requires std::integral<T>
  absl::StatusOr<std::vector<T>> RandomComposition(T n, int k,
                                                   T min_bucket_size = 1) {
    return under_test_.RandomComposition(n, k, min_bucket_size);
  }

 private:
  TypeUnderTest under_test_;
};

// -----------------------------------------------------------------------------
//  Helper matcher to test for distribution

// This is a *massively* simplified check to see if a distribution is
// approximately uniform.
MATCHER_P(IsApproximatelyUniformlyRandom, expected_number_of_buckets, "") {
  // This check is extremely naive. If the expected number of items in a bucket
  // is X, then this accepts as long as the number in each bucket is in the
  // range [X/3, 5*X/3]. (Those bounds were picked somewhat arbitrarily). This
  // moreso tests that each value "appears" than it checks for uniformity. This
  // can be improved to use Chi-Squared for a better test if needed.
  int expected_items_per_bucket = arg.size() / expected_number_of_buckets;
  int expected_lower = expected_items_per_bucket / 3;
  int expected_upper = 5 * expected_items_per_bucket / 3;

  if (expected_lower <= 5) {
    *result_listener
        << "This test is not effective if each bucket contains <= 5 items.";
    return false;
  }

  // arg_type                           == std::vector<Foo>
  // std::decay_t<arg_type>::value_type == Foo
  using sample_type = typename std::decay_t<arg_type>::value_type;
  absl::flat_hash_map<sample_type, int> count;
  for (const auto& item : arg) count[item]++;

  if (count.size() != expected_number_of_buckets) {
    *result_listener << "actual number of buckets (" << count.size()
                     << ") is not expected (" << expected_number_of_buckets
                     << ")";
    return false;
  }

  for (const auto& [bucket, num_occurrence] : count) {
    if (num_occurrence < expected_lower || num_occurrence > expected_upper) {
      *result_listener << testing::PrintToString(bucket) << " occurs "
                       << num_occurrence
                       << " times, which is outside the acceptable ["
                       << expected_lower << ", " << expected_upper << "] range";
      return false;
    }
  }

  return true;
}

// -----------------------------------------------------------------------------
//  RandomInteger()

TYPED_TEST_P(ValidInputRandomnessTests, RandomIntegerInTypicalCase) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomInteger(314, 314),
              moriarty::IsOkAndHolds(314));

  EXPECT_THAT(rng_under_test.RandomInteger(1), moriarty::IsOkAndHolds(0));

  EXPECT_THAT(
      rng_under_test.RandomInteger(1, 5),
      moriarty::IsOkAndHolds(testing::AllOf(testing::Ge(1), testing::Le(5))));

  EXPECT_THAT(rng_under_test.RandomInteger(-10, -5),
              moriarty::IsOkAndHolds(
                  testing::AllOf(testing::Ge(-10), testing::Le(-5))));

  EXPECT_THAT(
      rng_under_test.RandomInteger(4),
      moriarty::IsOkAndHolds(testing::AllOf(testing::Ge(0), testing::Lt(4))));
}

TYPED_TEST_P(InvalidInputRandomnessTests,
             RandomIntegerForInvalidRangesShouldFail) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomInteger(5, 3),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument));

  EXPECT_THAT(rng_under_test.RandomInteger(0),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument));

  EXPECT_THAT(rng_under_test.RandomInteger(-5),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument));
}

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomIntegerGivesApproximatelyTheRightDistribution) {
  TypeParam rng_under_test;

  {  // Double argument:
    std::vector<int> samples;
    for (int i = 0; i < 6000; i++) {
      MORIARTY_ASSERT_OK_AND_ASSIGN(
          int val, rng_under_test.RandomInteger(123, 134));
      samples.push_back(val);
    }

    EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(12));
  }

  {  // Single argument:
    std::vector<int> samples;
    for (int i = 0; i < 6000; i++) {
      MORIARTY_ASSERT_OK_AND_ASSIGN(int val,
                                             rng_under_test.RandomInteger(13));
      samples.push_back(val);
    }

    EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(13));
  }
}

// -----------------------------------------------------------------------------
//  Shuffle()

TYPED_TEST_P(ValidInputRandomnessTests, ShufflingAnEmptyArrayIsSuccessful) {
  TypeParam rng_under_test;

  std::vector<int> empty;
  MORIARTY_ASSERT_OK(rng_under_test.Shuffle(empty));
  EXPECT_THAT(empty, testing::IsEmpty());
}

TYPED_TEST_P(ValidInputRandomnessTests, ShufflePermutesElements) {
  TypeParam rng_under_test;

  // Shuffle integers
  std::vector<int> int_vals = {1, 22, 333, 4444, 55555};
  MORIARTY_ASSERT_OK(rng_under_test.Shuffle(int_vals));
  EXPECT_THAT(int_vals, testing::UnorderedElementsAre(1, 22, 333, 4444, 55555));

  // Shuffle strings
  std::vector<std::string> str_vals = {"these", "are", "some", "words"};
  MORIARTY_ASSERT_OK(rng_under_test.Shuffle(str_vals));
  EXPECT_THAT(str_vals,
              testing::UnorderedElementsAre("these", "are", "some", "words"));
}

TYPED_TEST_P(ValidInputRandomnessTests,
             ShuffleGivesApproximatelyTheRightDistribution) {
  TypeParam rng_under_test;

  std::vector<std::vector<int>> samples;
  std::vector<int> v = {1, 2, 3, 4};
  for (int i = 0; i < 6000; i++) {
    MORIARTY_ASSERT_OK(rng_under_test.Shuffle(v));
    samples.push_back(v);
  }

  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(24));
}

// -----------------------------------------------------------------------------
//  RandomElement()

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomElementGivesApproximatelyTheRightDistribution) {
  TypeParam rng_under_test;

  std::vector<int> samples;
  std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  for (int i = 0; i < 6000; i++) {
    MORIARTY_ASSERT_OK_AND_ASSIGN(int val,
                                           rng_under_test.RandomElement(v));
    samples.push_back(val);
  }

  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(10));
}

TYPED_TEST_P(ValidInputRandomnessTests, RandomElementGivesSomeValueFromArray) {
  TypeParam rng_under_test;

  std::vector<int> v = {123456, 456789, 789123};
  EXPECT_THAT(rng_under_test.RandomElement(v),
              moriarty::IsOkAndHolds(testing::AnyOfArray(v)));
}

TYPED_TEST_P(InvalidInputRandomnessTests,
             RandomElementFromAnEmptyContainerShouldFail) {
  TypeParam rng_under_test;

  std::vector<int> empty;
  EXPECT_THAT(rng_under_test.RandomElement(empty),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 testing::HasSubstr("empty")));
}

// -----------------------------------------------------------------------------
//  RandomElements()

TYPED_TEST_P(
    ValidInputRandomnessTests,
    RandomElementsWithReplacementGivesApproximatelyTheRightDistribution) {
  TypeParam rng_under_test;

  std::vector<std::vector<int>> samples;
  std::vector<int> v = {1, 2, 3, 4, 5};
  for (int i = 0; i < 6000; i++) {
    MORIARTY_ASSERT_OK_AND_ASSIGN(
        std::vector<int> vals,
        rng_under_test.RandomElementsWithReplacement(v, 3));
    samples.push_back(vals);
  }

  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(125));  // 125 = 5^3
}

TYPED_TEST_P(
    ValidInputRandomnessTests,
    RandomElementsWithoutReplacementGivesApproximatelyTheRightDistribution) {
  TypeParam rng_under_test;

  std::vector<std::vector<int>> samples;
  std::vector<int> v = {1, 2, 3, 4, 5};
  for (int i = 0; i < 6000; i++) {
    MORIARTY_ASSERT_OK_AND_ASSIGN(
        std::vector<int> vals,
        rng_under_test.RandomElementsWithoutReplacement(v, 3));
    samples.push_back(vals);
  }

  // 60 = (5 choose 3) * 3!
  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(60));
}

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomElementsGivesSomeValuesFromArray) {
  TypeParam rng_under_test;

  std::vector<std::string> v = {"123456", "456789", "789123"};
  EXPECT_THAT(rng_under_test.RandomElementsWithReplacement(v, 5),
              moriarty::IsOkAndHolds(testing::Each(testing::AnyOfArray(v))));
  EXPECT_THAT(rng_under_test.RandomElementsWithoutReplacement(v, 2),
              moriarty::IsOkAndHolds(testing::Each(testing::AnyOfArray(v))));
}

TYPED_TEST_P(ValidInputRandomnessTests, RandomElementsObeysDistinctOnlyPolicy) {
  TypeParam rng_under_test;

  std::vector<int> v = {11, 22, 33, 44};

  // Trial 20 times. Each trial has 4/64 chance of false positive.
  for (int i = 0; i < 20; i++) {
    EXPECT_THAT(
        rng_under_test.RandomElementsWithoutReplacement(v, 3),
        moriarty::IsOkAndHolds(
            testing::WhenSorted(testing::AnyOfArray<std::vector<int>>(
                {{11, 22, 33}, {11, 33, 44}, {11, 22, 44}, {22, 33, 44}}))));
  }
}

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomElementsWithReplacementCanContainDuplicates) {
  TypeParam rng_under_test;

  std::vector<int> v = {11, 22, 33, 44};

  // Only 4 elements and I'm querying for 6. Duplicates must be there!
  MORIARTY_EXPECT_OK(rng_under_test.RandomElementsWithReplacement(v, 6));
}

TYPED_TEST_P(InvalidInputRandomnessTests,
             RandomElementsWithNegativeInputShouldFail) {
  TypeParam rng_under_test;

  std::vector<int> v = {11, 22, 33, 44};

  EXPECT_THAT(rng_under_test.RandomElementsWithReplacement(v, -1),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 testing::HasSubstr("negative")));
  EXPECT_THAT(rng_under_test.RandomElementsWithoutReplacement(v, -1),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 testing::HasSubstr("negative")));
}

TYPED_TEST_P(InvalidInputRandomnessTests,
             RandomElementsFromAnEmptyContainerShouldFail) {
  TypeParam rng_under_test;

  std::vector<std::string> empty;
  EXPECT_THAT(rng_under_test.RandomElementsWithReplacement(empty, 1),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 testing::HasSubstr("empty")));
  EXPECT_THAT(rng_under_test.RandomElementsWithoutReplacement(empty, 1),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 testing::HasSubstr("size 0")));
}

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomElementsFromAnEmptyContainerShouldReturnNothing) {
  TypeParam rng_under_test;

  std::vector<std::string> empty;
  EXPECT_THAT(rng_under_test.RandomElementsWithReplacement(empty, 0),
              moriarty::IsOkAndHolds(testing::IsEmpty()));
  EXPECT_THAT(rng_under_test.RandomElementsWithoutReplacement(empty, 0),
              moriarty::IsOkAndHolds(testing::IsEmpty()));
}

TYPED_TEST_P(InvalidInputRandomnessTests,
             RandomElementsAskingForTooManyDistinctShouldFail) {
  TypeParam rng_under_test;

  std::vector<std::string> v = {"123456", "456789", "789123"};
  EXPECT_THAT(rng_under_test.RandomElementsWithoutReplacement(v, 5),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 testing::HasSubstr("distinct")));
}

// -----------------------------------------------------------------------------
//  RandomPermutation()

TYPED_TEST_P(InvalidInputRandomnessTests,
             RandomPermutationOfNegativeSizeShouldFail) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomPermutation(-1),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 testing::HasSubstr("negative")));
}

TYPED_TEST_P(ValidInputRandomnessTests, RandomPermutationOfSizeZeroIsOk) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomPermutation(0),
              moriarty::IsOkAndHolds(testing::IsEmpty()));
}

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomPermutationGeneratesAPermutation) {
  TypeParam rng_under_test;

  EXPECT_THAT(
      rng_under_test.RandomPermutation(5),
      moriarty::IsOkAndHolds(testing::UnorderedElementsAre(0, 1, 2, 3, 4)));
  EXPECT_THAT(rng_under_test.RandomPermutation(5, 10),
              moriarty::IsOkAndHolds(
                  testing::UnorderedElementsAre(10, 11, 12, 13, 14)));
}

TYPED_TEST_P(ValidInputRandomnessTests, RandomPermutationGivesPermutation) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomPermutation(0),
              moriarty::IsOkAndHolds(testing::IsEmpty()));

  EXPECT_THAT(rng_under_test.RandomPermutation(1),
              moriarty::IsOkAndHolds(testing::UnorderedElementsAre(0)));

  EXPECT_THAT(
      rng_under_test.RandomPermutation(4),
      moriarty::IsOkAndHolds(testing::UnorderedElementsAre(0, 1, 2, 3)));

  EXPECT_THAT(
      rng_under_test.RandomPermutation(6),
      moriarty::IsOkAndHolds(testing::UnorderedElementsAre(0, 1, 2, 3, 4, 5)));
}

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomPermutationGivesApproximatelyCorrectDistribution) {
  TypeParam rng_under_test;

  std::vector<std::vector<int>> samples;
  for (int i = 0; i < 6000; i++) {
    MORIARTY_ASSERT_OK_AND_ASSIGN(std::vector<int> v,
                                           rng_under_test.RandomPermutation(3));
    samples.push_back(v);
  }

  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(6));
}

// -----------------------------------------------------------------------------
//  DistinctIntegers()

TYPED_TEST_P(ValidInputRandomnessTests,
             DistinctIntegersGivesApproximatelyTheRightDistribution) {
  TypeParam rng_under_test;

  std::vector<std::vector<int>> samples;
  for (int i = 0; i < 6000; i++) {
    MORIARTY_ASSERT_OK_AND_ASSIGN(
        std::vector<int> vals, rng_under_test.DistinctIntegers(8, 2));
    samples.push_back(vals);
  }

  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(56));  // 56 = 8 * 7
}

TYPED_TEST_P(InvalidInputRandomnessTests,
             DistinctIntegersWithNegativeNOrKShouldFail) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.DistinctIntegers(-1, 5),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 AllOf(testing::HasSubstr("n "),
                                       testing::HasSubstr("negative"))));
  EXPECT_THAT(rng_under_test.DistinctIntegers(10, -5),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 AllOf(testing::HasSubstr("k "),
                                       testing::HasSubstr("negative"))));
}

TYPED_TEST_P(InvalidInputRandomnessTests,
             DistinctIntegersWithKGreaterThanNShouldFail) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.DistinctIntegers(5, 10),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 testing::HasSubstr("range of size 5")));
}

TYPED_TEST_P(ValidInputRandomnessTests, DistinctIntegersWithZeroIsFine) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.DistinctIntegers(0, 0),
              moriarty::IsOkAndHolds(testing::IsEmpty()));
  EXPECT_THAT(rng_under_test.DistinctIntegers(5, 0),
              moriarty::IsOkAndHolds(testing::IsEmpty()));
}

TYPED_TEST_P(ValidInputRandomnessTests, DistinctIntegersDoesNotGiveDuplicates) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.DistinctIntegers(10, 10),
              moriarty::IsOkAndHolds(
                  testing::UnorderedElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9)));

  MORIARTY_ASSERT_OK_AND_ASSIGN(
      std::vector<int> vals, rng_under_test.DistinctIntegers(50, 30));
  EXPECT_THAT(absl::flat_hash_set<int>(vals.begin(), vals.end()),
              testing::SizeIs(30));
}

TYPED_TEST_P(ValidInputRandomnessTests,
             DistinctIntegersOffsetWorksAppropriately) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.DistinctIntegers(10000, 100, 1234),
              moriarty::IsOkAndHolds(testing::Each(testing::AllOf(
                  testing::Ge(1234), testing::Le(10000 + 1234)))));
}

// -----------------------------------------------------------------------------
//  RandomComposition()

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomCompositionGivesApproximatelyCorrectDistribution) {
  TypeParam rng_under_test;

  std::vector<std::vector<int>> samples;
  for (int i = 0; i < 6000; i++) {
    MORIARTY_ASSERT_OK_AND_ASSIGN(
        std::vector<int> v, rng_under_test.RandomComposition(5, 3));
    samples.push_back(v);
  }

  EXPECT_THAT(samples, IsApproximatelyUniformlyRandom(6));
}

TYPED_TEST_P(InvalidInputRandomnessTests,
             RandomCompositionWithNegativeArgumentShouldFail) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomComposition(-1, 5),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 AllOf(testing::HasSubstr("n "),
                                       testing::HasSubstr("negative"))));
  EXPECT_THAT(rng_under_test.RandomComposition(10, -5),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 AllOf(testing::HasSubstr("k "),
                                       testing::HasSubstr("positive"))));
  EXPECT_THAT(rng_under_test.RandomComposition(10, 5, -1),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 testing::HasSubstr("negative")));
}

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomCompositionWithZeroBucketsAndZeroSumIsFine) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomComposition(0, 0, 0),
              moriarty::IsOkAndHolds(testing::IsEmpty()));
}

TYPED_TEST_P(InvalidInputRandomnessTests,
             RandomCompositionWithZeroBucketsAndNonZeroSumFails) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomComposition(5, 0),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 testing::HasSubstr("positive")));
}

TYPED_TEST_P(InvalidInputRandomnessTests,
             RandomCompositionWithTooLargeMinBucketShouldFail) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomComposition(4, 2, 3),
              moriarty::StatusIs(absl::StatusCode::kInvalidArgument,
                                 testing::HasSubstr("bucket")));
}

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomCompositionWithExactMinBucketIsFine) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomComposition(10, 5, 2),
              moriarty::IsOkAndHolds(std::vector<int>{2, 2, 2, 2, 2}));
}

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomCompositionRespectsMinBucketSize) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomComposition(40, 6, 2),
              moriarty::IsOkAndHolds(testing::Each(testing::Ge(2))));
}

TYPED_TEST_P(ValidInputRandomnessTests,
             RandomCompositionReturnsTheRightNumberOfBuckets) {
  TypeParam rng_under_test;

  EXPECT_THAT(rng_under_test.RandomComposition(40, 6, 2),
              moriarty::IsOkAndHolds(testing::SizeIs(6)));
  EXPECT_THAT(rng_under_test.RandomComposition(400, 3, 0),
              moriarty::IsOkAndHolds(testing::SizeIs(3)));
}

TYPED_TEST_P(ValidInputRandomnessTests, RandomCompositionHasCorrectTotalSum) {
  TypeParam rng_under_test;

  {
    MORIARTY_ASSERT_OK_AND_ASSIGN(
        std::vector<int> vals, rng_under_test.RandomComposition(40, 6, 2));
    EXPECT_EQ(absl::c_accumulate(vals, 0), 40);
  }
  {
    MORIARTY_ASSERT_OK_AND_ASSIGN(
        std::vector<int> vals, rng_under_test.RandomComposition(400, 3, 0));
    EXPECT_EQ(absl::c_accumulate(vals, 0), 400);
  }
  {
    MORIARTY_ASSERT_OK_AND_ASSIGN(
        std::vector<int> vals, rng_under_test.RandomComposition(555, 10));
    EXPECT_EQ(absl::c_accumulate(vals, 0), 555);
  }
}

// -----------------------------------------------------------------------------
//  googletest Registration

REGISTER_TYPED_TEST_SUITE_P(
    ValidInputRandomnessTests, DistinctIntegersDoesNotGiveDuplicates,
    DistinctIntegersGivesApproximatelyTheRightDistribution,
    DistinctIntegersOffsetWorksAppropriately, DistinctIntegersWithZeroIsFine,
    RandomCompositionGivesApproximatelyCorrectDistribution,
    RandomCompositionHasCorrectTotalSum, RandomCompositionRespectsMinBucketSize,
    RandomCompositionReturnsTheRightNumberOfBuckets,
    RandomCompositionWithExactMinBucketIsFine,
    RandomCompositionWithZeroBucketsAndZeroSumIsFine,
    RandomElementGivesApproximatelyTheRightDistribution,
    RandomElementGivesSomeValueFromArray,
    RandomElementsFromAnEmptyContainerShouldReturnNothing,
    RandomElementsGivesSomeValuesFromArray,
    RandomElementsObeysDistinctOnlyPolicy,
    RandomElementsWithReplacementCanContainDuplicates,
    RandomElementsWithReplacementGivesApproximatelyTheRightDistribution,
    RandomElementsWithoutReplacementGivesApproximatelyTheRightDistribution,
    RandomIntegerGivesApproximatelyTheRightDistribution,
    RandomIntegerInTypicalCase, RandomPermutationGeneratesAPermutation,
    RandomPermutationGivesApproximatelyCorrectDistribution,
    RandomPermutationGivesPermutation, RandomPermutationOfSizeZeroIsOk,
    ShufflingAnEmptyArrayIsSuccessful, ShufflePermutesElements,
    ShuffleGivesApproximatelyTheRightDistribution);

REGISTER_TYPED_TEST_SUITE_P(InvalidInputRandomnessTests,
                            DistinctIntegersWithKGreaterThanNShouldFail,
                            DistinctIntegersWithNegativeNOrKShouldFail,
                            RandomCompositionWithNegativeArgumentShouldFail,
                            RandomCompositionWithTooLargeMinBucketShouldFail,
                            RandomCompositionWithZeroBucketsAndNonZeroSumFails,
                            RandomElementFromAnEmptyContainerShouldFail,
                            RandomElementsAskingForTooManyDistinctShouldFail,
                            RandomElementsFromAnEmptyContainerShouldFail,
                            RandomElementsWithNegativeInputShouldFail,
                            RandomIntegerForInvalidRangesShouldFail,
                            RandomPermutationOfNegativeSizeShouldFail);

}  // namespace moriarty_testing

#endif  // MORIARTY_SRC_TESTING_RANDOM_TEST_UTIL_H_
