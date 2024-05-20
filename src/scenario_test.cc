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

#include "src/scenario.h"


#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/property.h"

namespace moriarty {

bool operator==(const Property& lhs, const Property& rhs) {
  return lhs.category == rhs.category && lhs.descriptor == rhs.descriptor &&
         lhs.enforcement == rhs.enforcement;
}

namespace {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

TEST(ScenarioTest, GeneralPropertiesGetAndSetAreConnectedAppropriately) {
  Scenario S = Scenario()
                   .WithGeneralProperty({.category = "A", .descriptor = "a"})
                   .WithGeneralProperty({.category = "B", .descriptor = "b"})
                   .WithGeneralProperty({.category = "C", .descriptor = "c"});

  EXPECT_THAT(S.GetGeneralProperties(),
              ElementsAre(Property({.category = "A", .descriptor = "a"}),
                          Property({.category = "B", .descriptor = "b"}),
                          Property({.category = "C", .descriptor = "c"})));
}

TEST(ScenarioTest, TypeSpecificPropertiesGetAndSetAreConnectedAppropriately) {
  Scenario S = Scenario()
                   .WithTypeSpecificProperty(
                       "MInteger", {.category = "A", .descriptor = "a"})
                   .WithTypeSpecificProperty(
                       "MInteger", {.category = "B", .descriptor = "b"})
                   .WithTypeSpecificProperty(
                       "MInteger", {.category = "C", .descriptor = "c"})
                   .WithTypeSpecificProperty(
                       "MOther", {.category = "X", .descriptor = "x"});

  EXPECT_THAT(S.GetTypeSpecificProperties("MInteger"),
              ElementsAre(Property({.category = "A", .descriptor = "a"}),
                          Property({.category = "B", .descriptor = "b"}),
                          Property({.category = "C", .descriptor = "c"})));
  EXPECT_THAT(S.GetTypeSpecificProperties("MOther"),
              ElementsAre(Property({.category = "X", .descriptor = "x"})));
  EXPECT_THAT(S.GetTypeSpecificProperties("MString"), IsEmpty());
}

}  // namespace
}  // namespace moriarty
