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

#include "src/variables/constraints/string_constraints.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_set.h"

namespace moriarty {
namespace {

using ::testing::Not;
using ::testing::SizeIs;

MATCHER(HasDuplicateLetter,
        negation ? "has no duplicate letters" : "has duplicate letters") {
  absl::flat_hash_set<char> seen;
  for (char c : arg) {
    auto [it, inserted] = seen.insert(c);
    if (!inserted) {
      *result_listener << "duplicate letter = " << c;
      return true;
    }
  }
  return false;
}

// These tests below are simply safety checks to ensure the alphabets are
// correctly typed and not accidentally modified later.
TEST(AlphabetTest, CommonAlphabetsDoNotHaveDuplicatedLetters) {
  EXPECT_THAT(Alphabet::Letters().GetAlphabet(), Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::UpperCase().GetAlphabet(), Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::LowerCase().GetAlphabet(), Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::Numbers().GetAlphabet(), Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::AlphaNumeric().GetAlphabet(),
              Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::UpperAlphaNumeric().GetAlphabet(),
              Not(HasDuplicateLetter()));
  EXPECT_THAT(Alphabet::LowerAlphaNumeric().GetAlphabet(),
              Not(HasDuplicateLetter()));
}

TEST(AlphabetTest, CommonAlphabetsHaveTheAppropriateNumberOfElements) {
  EXPECT_THAT(Alphabet::Letters().GetAlphabet(), SizeIs(26 + 26));
  EXPECT_THAT(Alphabet::UpperCase().GetAlphabet(), SizeIs(26));
  EXPECT_THAT(Alphabet::LowerCase().GetAlphabet(), SizeIs(26));
  EXPECT_THAT(Alphabet::Numbers().GetAlphabet(), SizeIs(10));
  EXPECT_THAT(Alphabet::AlphaNumeric().GetAlphabet(), SizeIs(26 + 26 + 10));
  EXPECT_THAT(Alphabet::UpperAlphaNumeric().GetAlphabet(), SizeIs(26 + 10));
  EXPECT_THAT(Alphabet::LowerAlphaNumeric().GetAlphabet(), SizeIs(26 + 10));
}

TEST(AlphabetTest, BasicConstructorShouldReturnExactAlphabet) {
  EXPECT_EQ(Alphabet("abc").GetAlphabet(), "abc");
  EXPECT_EQ(Alphabet("AbC").GetAlphabet(), "AbC");
  EXPECT_EQ(Alphabet("A\tC").GetAlphabet(), "A\tC");
  EXPECT_EQ(Alphabet("AAA").GetAlphabet(), "AAA");
}

}  // namespace
}  // namespace moriarty
