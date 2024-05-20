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

#include "src/constraint_values.h"

#include <cstdint>
#include <string>

#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
#include "src/internal/universe.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/testing/mtest_type.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace {

using ::moriarty::moriarty_internal::Universe;
using ::moriarty::moriarty_internal::ValueSet;
using ::moriarty::moriarty_internal::VariableSet;
using ::moriarty_testing::MTestType;

TEST(ConstraintValuesTest, GetValueObtainsVariableValue) {
  Universe u;
  ValueSet values;
  std::string var_name = "var";
  int64_t value = 1;

  values.Set<MInteger>(var_name, value);
  u.SetConstValueSet(&values);

  ConstraintValues cv(&u);
  EXPECT_EQ(cv.GetValue<MInteger>(var_name), value);
}

TEST(ConstraintValuesDeathTest, GetValueCrashesOnNotGeneratedValue) {
  Universe u;
  VariableSet variables;
  ValueSet values;
  std::string var_name = "var";

  ABSL_CHECK_OK(variables.AddVariable(
      var_name, MTestType().SetMultiplier(MInteger().Is(2))));
  u.SetMutableVariableSet(&variables);
  u.SetMutableValueSet(&values);

  ConstraintValues cv(&u);
  EXPECT_DEATH(cv.GetValue<MInteger>("var"), "NOT_FOUND: Value");
}

TEST(ConstraintValuesDeathTest, GetValueCrashesOnInvalidVariableName) {
  Universe u;
  ValueSet values;
  VariableSet variables;

  u.SetConstVariableSet(&variables);
  u.SetConstValueSet(&values);
  ConstraintValues cv(&u);

  EXPECT_DEATH(cv.GetValue<MInteger>("non_existing_var"),
               "NOT_FOUND: Unknown variable");
}

TEST(ConstraintValuesDeathTest, GetValueCrashesOnInvalidType) {
  Universe u;
  ValueSet values;
  VariableSet variables;

  values.Set<MString>("X", "string");
  u.SetConstVariableSet(&variables);
  u.SetConstValueSet(&values);
  ConstraintValues cv(&u);

  EXPECT_DEATH(cv.GetValue<MInteger>("X"), "Unable to cast X");
}

}  // namespace
}  // namespace moriarty
