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

#include "src/variables/constraints/container_constraints.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/librarian/test_utils.h"
#include "src/variables/constraints/numeric_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::Context;
using ::moriarty_testing::GeneratedValuesAre;
using ::moriarty_testing::GenerateLots;
using ::testing::AllOf;
using ::testing::Each;
using ::testing::Ge;
using ::testing::Le;
using ::testing::status::IsOkAndHolds;

TEST(ContainerConstraintsTest, LengthConstraintsAreCorrect) {
  EXPECT_THAT(Length(10).GetConstraints(), GeneratedValuesAre(10));
  EXPECT_THAT(GenerateLots(Length("2 * N").GetConstraints(),
                           Context().WithValue<MInteger>("N", 7)),
              IsOkAndHolds(Each(14)));
  EXPECT_THAT(GenerateLots(Length(AtLeast("X"), AtMost(15)).GetConstraints(),
                           Context().WithValue<MInteger>("X", 3)),
              IsOkAndHolds(Each(AllOf(Ge(3), Le(15)))));
}

TEST(ContainerConstraintsTest, ElementsConstraintsAreCorrect) {
  EXPECT_THAT(Elements<MInteger>(Between(1, 10)).GetConstraints(),
              GeneratedValuesAre(AllOf(Ge(1), Le(10))));
  EXPECT_THAT(GenerateLots(
                  Elements<MInteger>(AtLeast("X"), AtMost(15)).GetConstraints(),
                  Context().WithValue<MInteger>("X", 3)),
              IsOkAndHolds(Each(AllOf(Ge(3), Le(15)))));
}

}  // namespace
}  // namespace moriarty
