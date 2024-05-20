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

#include "src/internal/variable_name_utils.h"

#include <optional>

#include "gtest/gtest.h"

namespace moriarty {
namespace moriarty_internal {
namespace {

TEST(VariableNameUtilsTest, BaseVariableNameReturnedCorrectly) {
  EXPECT_EQ(BaseVariableName("abcd.efgh.ijkl"), "abcd");
  EXPECT_EQ(BaseVariableName("abcd"), "abcd");
  EXPECT_EQ(BaseVariableName(""), "");
}

TEST(VariableNameUtilsTest, SubvariableNameReturnedCorrectly) {
  EXPECT_EQ(SubvariableName("abcd.efgh.ijkl"), "efgh.ijkl");
  EXPECT_EQ(SubvariableName("abcd"), std::nullopt);
  EXPECT_EQ(SubvariableName(""), std::nullopt);
}

TEST(VariableNameUtilsTest, ConstructVariableNameCorrectReturn) {
  EXPECT_EQ(ConstructVariableName("abcd", "efgh.ijkl"), "abcd.efgh.ijkl");
  EXPECT_EQ(ConstructVariableName({.base_variable_name = "abcd"}), "abcd");
  EXPECT_EQ(ConstructVariableName({.base_variable_name = "abcd",
                                   .subvariable_name = "efgh.ijkl"}),
            "abcd.efgh.ijkl");
}

TEST(VariableNameUtilsTest, HasHasSubvariableReturnedCorrectly) {
  EXPECT_FALSE(HasSubvariable("abcd"));
  EXPECT_TRUE(HasSubvariable("efgh.ijkl"));
}

TEST(VariableNameUtilsTest, CreateVariableNameBreakdownReturnedCorrectly) {
  EXPECT_EQ(CreateVariableNameBreakdown("abcd.efgh.ijkl"),
            VariableNameBreakdown({.base_variable_name = "abcd",
                                   .subvariable_name = "efgh.ijkl"}));
  EXPECT_EQ(CreateVariableNameBreakdown("abcd"),
            VariableNameBreakdown({.base_variable_name = "abcd"}));
}

}  // namespace
}  // namespace moriarty_internal
}  // namespace moriarty
