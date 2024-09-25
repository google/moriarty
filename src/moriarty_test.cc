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

#include "src/moriarty.h"

#include <optional>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "src/test_case.h"
#include "src/testing/exporter_test_util.h"
#include "src/testing/generator_test_util.h"
#include "src/testing/importer_test_util.h"
#include "src/testing/mtest_type.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::ExampleTestCase;
using ::moriarty_testing::IsUnsatisfiedConstraint;
using ::moriarty_testing::IsValueNotFound;
using ::moriarty_testing::MTestType;
using ::moriarty_testing::SingleIntegerExporter;
using ::moriarty_testing::SingleIntegerFromVectorImporter;
using ::moriarty_testing::SingleIntegerGenerator;
using ::moriarty_testing::SingleStringGenerator;
using ::moriarty_testing::TwoIntegerExporter;
using ::moriarty_testing::TwoIntegerGenerator;
using ::moriarty_testing::TwoIntegerGeneratorWithRandomness;
using ::moriarty_testing::TwoTestTypeWrongTypeExporter;
using ::moriarty_testing::TwoVariableFromVectorImporter;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::HasSubstr;
using ::testing::Le;
using ::testing::SizeIs;

TEST(MoriartyTest, SetNumCasesWorksForValidInput) {
  moriarty::Moriarty M;

  // The following will crash if the seed is invalid
  M.SetNumCases(0);
  M.SetNumCases(10);

  // TODO(b/182810006): Add tests that check at least this many cases are
  // generated once Generators are added to Moriarty.
}

TEST(MoriartyDeathTest, SetNumCasesWithNegativeInputShouldCrash) {
  moriarty::Moriarty M;
  EXPECT_DEATH(M.SetNumCases(-1), "num_cases must be non-negative");
  EXPECT_DEATH(M.SetNumCases(-100), "num_cases must be non-negative");
}

// TODO(b/182810006): Update seed when seed/name requirements are enabled
TEST(MoriartyTest, SetSeedWorksForValidInput) {
  moriarty::Moriarty M;

  // The following will crash if the seed is invalid
  M.SetSeed("abcde0123456789");
}

TEST(MoriartyDeathTest, ShouldSeedWithInvalidInputShouldCrash) {
  moriarty::Moriarty M;

  EXPECT_DEATH(M.SetSeed(""), "seed's length must be at least");
  EXPECT_DEATH(M.SetSeed("abcde"), "seed's length must be at least");
}

TEST(MoriartyTest, AddVariableWorks) {
  moriarty::Moriarty M;

  M.AddVariable("A", MTestType());
  M.AddVariable("B", MTestType());

  M.AddVariable("C", MTestType()).AddVariable("D", MTestType());
}

TEST(MoriartyDeathTest, AddTwoVariablesWithTheSameNameShouldCrash) {
  moriarty::Moriarty M;

  EXPECT_DEATH(M.AddVariable("X", MTestType()).AddVariable("X", MTestType()),
               "already added");
}

TEST(MoriartyTest,
     ExportAllTestCasesShouldExportCasesProperlyWithASingleGenerator) {
  using Case = ExampleTestCase;

  moriarty::Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddGenerator("Single Variable", SingleIntegerGenerator());
  M.GenerateTestCases();

  std::vector<Case> test_cases;
  SingleIntegerExporter exporter(&test_cases);
  M.ExportTestCases(exporter);

  EXPECT_THAT(test_cases, ElementsAre(Case({.n = 0})));
}

TEST(MoriartyTest,
     ExportAllTestCasesShouldExportProperlyWithASingleGeneratorMultipleTimes) {
  using Case = ExampleTestCase;

  moriarty::Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddGenerator("Single Variable", SingleIntegerGenerator(), 3);
  M.GenerateTestCases();

  std::vector<Case> test_cases;
  SingleIntegerExporter exporter(&test_cases);
  M.ExportTestCases(exporter);

  EXPECT_THAT(test_cases,
              ElementsAre(Case({.n = 0}), Case({.n = 0}), Case({.n = 0})));
}

TEST(MoriartyTest,
     ExportAllTestCasesShouldExportCasesProperlyWithMultipleGenerators) {
  using Case = ExampleTestCase;

  moriarty::Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddGenerator("Gen 1", TwoIntegerGenerator(1, 11));
  M.AddGenerator("Gen 2", TwoIntegerGenerator(2, 22));
  M.AddGenerator("Gen 3", TwoIntegerGenerator(3, 33));
  M.GenerateTestCases();

  std::vector<Case> test_cases;
  TwoIntegerExporter exporter(&test_cases);
  M.ExportTestCases(exporter);

  EXPECT_THAT(test_cases,
              ElementsAre(Case({.r = 1, .s = 11}), Case({.r = 1, .s = 11}),
                          Case({.r = 2, .s = 22}), Case({.r = 2, .s = 22}),
                          Case({.r = 3, .s = 33}), Case({.r = 3, .s = 33})));
}

TEST(
    MoriartyTest,
    ExportAllTestCasesShouldExportProperlyWithMultipleGeneratorsMultipleTimes) {
  using Case = ExampleTestCase;

  moriarty::Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddGenerator("Gen 1", TwoIntegerGenerator(1, 11), 2);
  M.AddGenerator("Gen 2", TwoIntegerGenerator(2, 22), 3);
  M.GenerateTestCases();

  std::vector<Case> test_cases;
  TwoIntegerExporter exporter(&test_cases);
  M.ExportTestCases(exporter);

  EXPECT_THAT(test_cases,
              ElementsAre(Case({.r = 1, .s = 11}), Case({.r = 1, .s = 11}),
                          Case({.r = 1, .s = 11}), Case({.r = 1, .s = 11}),
                          Case({.r = 2, .s = 22}), Case({.r = 2, .s = 22}),
                          Case({.r = 2, .s = 22}), Case({.r = 2, .s = 22}),
                          Case({.r = 2, .s = 22}), Case({.r = 2, .s = 22})));
}

TEST(MoriartyDeathTest, ExportAskingForNonExistentVariableShouldCrash) {
  using Case = ExampleTestCase;

  moriarty::Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddGenerator("Gen 1", TwoIntegerGenerator(1, 11));
  M.GenerateTestCases();

  std::vector<Case> test_cases;
  SingleIntegerExporter exporter(&test_cases);

  // Exporter requests N, but only X and Y are set...
  EXPECT_DEATH({ M.ExportTestCases(exporter); }, "ValueNotFound N");
}

TEST(MoriartyDeathTest, ExportAskingForVariableOfTheWrongTypeShouldCrash) {
  using Case = ExampleTestCase;

  moriarty::Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddGenerator("Gen 1", TwoIntegerGenerator(1, 11));
  M.GenerateTestCases();

  std::vector<Case> test_cases;
  TwoTestTypeWrongTypeExporter exporter(&test_cases);

  // Exporter requests for FakeDataType, but R and S are ints...
  EXPECT_DEATH({ M.ExportTestCases(exporter); }, "Unable to cast");
}

TEST(MoriartyTest, ExportCasesShouldExportProperMetadata) {
  using Case = ExampleTestCase;

  moriarty::Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddGenerator("Gen 1", TwoIntegerGenerator(1, 11), 2);
  M.AddGenerator("Gen 2", TwoIntegerGenerator(2, 22), 3);
  M.GenerateTestCases();

  std::vector<Case> test_cases;
  TwoIntegerExporter exporter(&test_cases);
  M.ExportTestCases(exporter);

  {  // int overall_test_case_number;
    std::vector<int> test_case_number;
    for (const Case& c : test_cases) {
      test_case_number.push_back(c.metadata.GetTestCaseNumber());
    }
    EXPECT_THAT(test_case_number, ElementsAre(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
  }

  {  // std::string generator_name;
    std::vector<std::string> generator_name;
    for (const Case& c : test_cases) {
      generator_name.push_back(
          c.metadata.GetGeneratorMetadata()->generator_name);
    }
    EXPECT_THAT(generator_name,
                ElementsAre("Gen 1", "Gen 1", "Gen 1", "Gen 1", "Gen 2",
                            "Gen 2", "Gen 2", "Gen 2", "Gen 2", "Gen 2"));
  }

  {
    //  int generator_iteration;
    std::vector<int> generator_iteration;
    for (const Case& c : test_cases) {
      generator_iteration.push_back(
          c.metadata.GetGeneratorMetadata()->generator_iteration);
    }
    EXPECT_THAT(generator_iteration, ElementsAre(1, 1, 2, 2, 1, 1, 2, 2, 3, 3));
  }

  {  //  int case_number_in_generator;
    std::vector<int> case_number_in_generator;
    for (const Case& c : test_cases) {
      case_number_in_generator.push_back(
          c.metadata.GetGeneratorMetadata()->case_number_in_generator);
    }
    EXPECT_THAT(case_number_in_generator,
                ElementsAre(1, 2, 1, 2, 1, 2, 1, 2, 1, 2));
  }
}

TEST(MoriartyTest, GeneralConstraintsSetValueAreConsideredInGenerators) {
  moriarty::Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddVariable("N", MInteger().Is(5));
  M.AddGenerator("Single Variable", SingleIntegerGenerator());

  // Internally, this value is being set to 0, so it should fail since we said
  // here it must be exactly 5.
  EXPECT_DEATH({ M.GenerateTestCases(); }, "no viable value found");
}

TEST(MoriartyTest, GeneralConstraintsAreConsideredInGenerators) {
  using Case = ExampleTestCase;

  moriarty::Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddVariable("R", MInteger().Between(3, 50));  // X is generated between 1,10
  M.AddGenerator("Two Var", TwoIntegerGeneratorWithRandomness(), 10000);
  M.GenerateTestCases();

  std::vector<ExampleTestCase> test_cases;
  TwoIntegerExporter exporter(&test_cases);
  M.ExportTestCases(exporter);

  // General says 3 <= r <= 50. Generator says 1 <= r <= 10.
  for (const Case& c : test_cases) {
    // Using ASSERT so we don't get too many errors.
    ASSERT_GE(c.r, 3);
    ASSERT_LE(c.r, 10);
  }
}

TEST(MoriartyTest, ImporterShouldProperlyImportData) {
  using Case = ExampleTestCase;
  Moriarty M;
  MORIARTY_EXPECT_OK(
      M.ImportTestCases(SingleIntegerFromVectorImporter({1, 2, 3, 4})));
  std::vector<ExampleTestCase> test_cases;
  SingleIntegerExporter exporter(&test_cases);
  M.ExportTestCases(exporter);

  EXPECT_THAT(test_cases, ElementsAre(Case({.n = 1}), Case({.n = 2}),
                                      Case({.n = 3}), Case({.n = 4})));
}

TEST(MoriartyTest, ValidateAllTestCasesWorksWhenAllVariablesAreValid) {
  Moriarty M;
  M.AddVariable("N", MInteger().Between(1, 5));
  MORIARTY_EXPECT_OK(
      M.ImportTestCases(SingleIntegerFromVectorImporter({1, 2, 3, 4, 5})));
  MORIARTY_EXPECT_OK(M.TryValidateTestCases());
}

TEST(MoriartyTest, ValidateAllTestCasesFailsWhenSingleVariableInvalid) {
  Moriarty M;
  M.AddVariable("N", MInteger().Between(1, 3));
  MORIARTY_EXPECT_OK(
      M.ImportTestCases(SingleIntegerFromVectorImporter({1, 2, 3, 4, 5})));

  EXPECT_THAT(M.TryValidateTestCases(), IsUnsatisfiedConstraint("Case 4"));
}

TEST(MoriartyTest, ValidateAllTestCasesFailsWhenSomeVariableInvalid) {
  Moriarty M;
  M.AddVariable("R", MInteger().Between(1, 3))
      .AddVariable("S", MInteger().Between(10, 30));
  MORIARTY_EXPECT_OK(M.ImportTestCases(
      TwoVariableFromVectorImporter({{1, 11}, {2, 22}, {3, 33}, {4, 44}})));

  EXPECT_THAT(M.TryValidateTestCases(), IsUnsatisfiedConstraint("Case 3"));
}

TEST(MoriartyTest, ValidateAllTestCasesFailsIfAVariableIsMissing) {
  Moriarty M;
  M.AddVariable("R", MInteger().Between(1, 3))
      .AddVariable("X", MInteger().Between(10, 30));  // Importer imports "S"
  MORIARTY_EXPECT_OK(M.ImportTestCases(
      TwoVariableFromVectorImporter({{1, 11}, {2, 22}, {3, 33}, {4, 44}})));

  EXPECT_THAT(M.TryValidateTestCases(), IsValueNotFound("X"));
}

TEST(MoriartyTest, ApproximateGenerationLimitStopsGenerationEarly) {
  using Case = ExampleTestCase;

  moriarty::Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddGenerator("Generator", TwoIntegerGenerator(1, 11), 50);

  // Each run of the generator creates 2 test cases, and each test case has 2
  // integers (thus, size of 4 per test case). On the 8th call to the generator,
  // we go from size 28 to size 32. Since 32 >= 30, we stop!
  M.SetApproximateGenerationLimit(30);
  M.GenerateTestCases();

  std::vector<Case> test_cases;
  TwoIntegerExporter exporter(&test_cases);
  M.ExportTestCases(exporter);

  EXPECT_THAT(test_cases, SizeIs(16));
}

TEST(MoriartyTest, ApproximateGenerationLimitTruncatesTheSizeOfArrays) {
  using Case = ExampleTestCase;

  moriarty::Moriarty M;
  M.SetSeed("abcde0123456789");
  M.AddGenerator("Generator", SingleStringGenerator(), 100);

  // This limits the total size of strings/arrays to be 30.
  M.SetApproximateGenerationLimit(30);
  M.GenerateTestCases();

  std::vector<Case> test_cases;
  moriarty_testing::SingleStringExporter exporter(&test_cases);
  M.ExportTestCases(exporter);

  EXPECT_THAT(test_cases, Each(Field(&moriarty_testing::ExampleTestCase::str,
                                     SizeIs(Le(30)))));
}

TEST(MoriartyTest, VariableNameValidationShouldWork) {
  EXPECT_OK(Moriarty().TryAddVariable("good", MInteger()));
  EXPECT_OK(Moriarty().TryAddVariable("a1_b", MInteger()));
  EXPECT_OK(Moriarty().TryAddVariable("AbC_3c", MInteger()));

  EXPECT_THAT(Moriarty().TryAddVariable("", MInteger()),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("empty")));

  // TODO(darcybest): Re-enable these tests once our other user updates their
  // workflow.

  // EXPECT_THAT(Moriarty().TryAddVariable("1", MInteger()),
  //             StatusIs(absl::StatusCode::kInvalidArgument,
  //             HasSubstr("start")));
  // EXPECT_THAT(Moriarty().TryAddVariable("_", MInteger()),
  //             StatusIs(absl::StatusCode::kInvalidArgument,
  //             HasSubstr("start")));

  EXPECT_THAT(
      Moriarty().TryAddVariable(" ", MInteger()),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("A-Za-z0-9_")));
  EXPECT_THAT(
      Moriarty().TryAddVariable("$", MInteger()),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("A-Za-z0-9_")));
  EXPECT_THAT(
      Moriarty().TryAddVariable("a$", MInteger()),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("A-Za-z0-9_")));
  EXPECT_THAT(
      Moriarty().TryAddVariable("a b", MInteger()),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("A-Za-z0-9_")));
}

}  // namespace
}  // namespace moriarty
