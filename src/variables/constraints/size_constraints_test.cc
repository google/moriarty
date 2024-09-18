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

#include "src/variables/constraints/size_constraints.h"

#include "gtest/gtest.h"
#include "src/librarian/size_property.h"

namespace moriarty {
namespace {

TEST(SizeConstraintsTest, StaticConstructorsShouldWork) {
  EXPECT_EQ(SizeCategory::Any().GetCommonSize(), CommonSize::kAny);
  EXPECT_EQ(SizeCategory::Min().GetCommonSize(), CommonSize::kMin);
  EXPECT_EQ(SizeCategory::Tiny().GetCommonSize(), CommonSize::kTiny);
  EXPECT_EQ(SizeCategory::Small().GetCommonSize(), CommonSize::kSmall);
  EXPECT_EQ(SizeCategory::Medium().GetCommonSize(), CommonSize::kMedium);
  EXPECT_EQ(SizeCategory::Large().GetCommonSize(), CommonSize::kLarge);
  EXPECT_EQ(SizeCategory::Huge().GetCommonSize(), CommonSize::kHuge);
  EXPECT_EQ(SizeCategory::Max().GetCommonSize(), CommonSize::kMax);
}

TEST(SizeConstraintsTest, FromEnumShouldWork) {
  EXPECT_EQ(SizeCategory(CommonSize::kAny).GetCommonSize(), CommonSize::kAny);
  EXPECT_EQ(SizeCategory(CommonSize::kMin).GetCommonSize(), CommonSize::kMin);
  EXPECT_EQ(SizeCategory(CommonSize::kTiny).GetCommonSize(), CommonSize::kTiny);
  EXPECT_EQ(SizeCategory(CommonSize::kSmall).GetCommonSize(),
            CommonSize::kSmall);
  EXPECT_EQ(SizeCategory(CommonSize::kMedium).GetCommonSize(),
            CommonSize::kMedium);
  EXPECT_EQ(SizeCategory(CommonSize::kLarge).GetCommonSize(),
            CommonSize::kLarge);
  EXPECT_EQ(SizeCategory(CommonSize::kHuge).GetCommonSize(), CommonSize::kHuge);
  EXPECT_EQ(SizeCategory(CommonSize::kMax).GetCommonSize(), CommonSize::kMax);
}

TEST(SizeConstraintsTest, FromStringShouldWork) {
  EXPECT_EQ(SizeCategory("any").GetCommonSize(), CommonSize::kAny);
  EXPECT_EQ(SizeCategory("min").GetCommonSize(), CommonSize::kMin);
  EXPECT_EQ(SizeCategory("tiny").GetCommonSize(), CommonSize::kTiny);
  EXPECT_EQ(SizeCategory("small").GetCommonSize(), CommonSize::kSmall);
  EXPECT_EQ(SizeCategory("medium").GetCommonSize(), CommonSize::kMedium);
  EXPECT_EQ(SizeCategory("large").GetCommonSize(), CommonSize::kLarge);
  EXPECT_EQ(SizeCategory("huge").GetCommonSize(), CommonSize::kHuge);
  EXPECT_EQ(SizeCategory("max").GetCommonSize(), CommonSize::kMax);
}

}  // namespace
}  // namespace moriarty
