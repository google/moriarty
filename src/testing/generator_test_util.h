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

#ifndef MORIARTY_SRC_TESTING_GENERATOR_TEST_UTIL_H_
#define MORIARTY_SRC_TESTING_GENERATOR_TEST_UTIL_H_

#include <vector>

#include "absl/status/statusor.h"
#include "absl/types/span.h"
#include "src/generator.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/test_case.h"
#include "src/testing/mtest_type.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty_testing {

using ::moriarty_testing::MTestType;

// LINT.IfChange

// Generates one case with an integer N = 0
class SingleIntegerGenerator : public moriarty::Generator {
 public:
  void GenerateTestCases() override {
    AddTestCase().SetValue<moriarty::MInteger>("N", 0);
  }
};

// Generates several cases with a single string `str`
class SingleStringGenerator : public moriarty::Generator {
 public:
  void GenerateTestCases() override {
    moriarty::MString S = moriarty::MString().OfLength(1, 1000).WithAlphabet(
        moriarty::MString::kLowerCase);
    AddTestCase().ConstrainVariable("str", S);
    AddTestCase().ConstrainVariable("str", S);
    AddTestCase().ConstrainVariable("str", S);
  }
};

// Generates a single test cases.
// - One with MTestType: X = default, Y = default
class SimpleTestTypeGenerator : public moriarty::Generator {
 public:
  void GenerateTestCases() override {
    AddTestCase()
        .ConstrainVariable("X", MTestType())
        .ConstrainVariable("Y", MTestType());
  }
};

// Generates three cases.
// - One with MTestType: X = default, Y = default
// - One with MTestType: X = default, Y = 20
// - One with MTestType: X = 10, Y = 20
class TwoTestTypeGenerator : public moriarty::Generator {
 public:
  void GenerateTestCases() override {
    AddTestCase()
        .ConstrainVariable("X", MTestType())
        .ConstrainVariable("Y", MTestType());

    AddTestCase()
        .ConstrainVariable("X", MTestType())
        .ConstrainVariable("Y", MTestType().Is(20));

    AddTestCase()
        .ConstrainVariable("X", MTestType().Is(10))
        .ConstrainVariable("Y", MTestType().Is(20));
  }
};

// Generates two identical cases (r, s)
class TwoIntegerGenerator : public moriarty::Generator {
 public:
  explicit TwoIntegerGenerator(int r, int s) : r_(r), s_(s) {}
  void GenerateTestCases() override {
    AddTestCase()
        .SetValue<moriarty::MInteger>("R", r_)
        .SetValue<moriarty::MInteger>("S", s_);
    AddTestCase()
        .SetValue<moriarty::MInteger>("R", r_)
        .SetValue<moriarty::MInteger>("S", s_);
  }

 private:
  int r_;
  int s_;
};

// Generates four cases. Each is:
// - A random integer for R between 1 and 10.
// - A random integer for S between 8 and 20.
class TwoIntegerGeneratorWithRandomness : public moriarty::Generator {
 public:
  void GenerateTestCases() override {
    for (int i = 0; i < 4; i++) {
      AddTestCase()
          .ConstrainVariable("R", moriarty::MInteger().Between(1, 10))
          .ConstrainVariable("S", moriarty::MInteger().Between(8, 20));
    }
  }
};

// LINT.ThenChange(exporter_test_util.h)

// Adds a random engine to `generator`, then calls GenerateTestCases(). Returns
// a list of the test cases.>
template <typename T>
absl::StatusOr<std::vector<moriarty::moriarty_internal::ValueSet>>
RunGenerateForTest(T generator) {
  moriarty::moriarty_internal::GeneratorManager generator_manager(&generator);

  generator_manager.SetSeed({1, 2, 3});
  generator_manager.SetGeneralConstraints({});  // None
  generator.GenerateTestCases();

  return generator_manager.AssignValuesInAllTestCases();
}

}  // namespace moriarty_testing

#endif  // MORIARTY_SRC_TESTING_GENERATOR_TEST_UTIL_H_
