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

#include "src/simple_io.h"

#include <sstream>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "src/exporter.h"
#include "src/importer.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/test_case.h"
#include "src/testing/exporter_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::ExampleTestCase;
using ::moriarty_testing::GetExportedCases;
using ::moriarty_testing::TwoIntegerExporter;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::StrEq;
using ::testing::VariantWith;
using ::moriarty::StatusIs;

// -----------------------------------------------------------------------------
//  SimpleIO

TEST(SimpleIOTest, AddLineIsRetrievableViaLinesPerTestCase) {
  SimpleIO s;
  s.AddLine("hello", "world!").AddLine("how", StringLiteral("are"), "you?");

  EXPECT_THAT(
      s.LinesPerTestCase(),
      ElementsAre(ElementsAre(VariantWith<std::string>("hello"),
                              VariantWith<std::string>("world!")),
                  ElementsAre(VariantWith<std::string>("how"),
                              VariantWith<StringLiteral>(StringLiteral("are")),
                              VariantWith<std::string>("you?"))));
}

TEST(SimpleIOTest, AddHeaderLineIsRetrievableViaLinesInHeader) {
  SimpleIO s;
  s.AddHeaderLine("hello", "header!")
      .AddHeaderLine("how", StringLiteral("are"), "you?");

  EXPECT_THAT(
      s.LinesInHeader(),
      ElementsAre(ElementsAre(VariantWith<std::string>("hello"),
                              VariantWith<std::string>("header!")),
                  ElementsAre(VariantWith<std::string>("how"),
                              VariantWith<StringLiteral>(StringLiteral("are")),
                              VariantWith<std::string>("you?"))));
}

TEST(SimpleIOTest, AddHeaderLineIsRetrievableViaLinesInFooter) {
  SimpleIO s;
  s.AddFooterLine("hello", "footer!")
      .AddFooterLine("how", StringLiteral("are"), "you?");

  EXPECT_THAT(
      s.LinesInFooter(),
      ElementsAre(ElementsAre(VariantWith<std::string>("hello"),
                              VariantWith<std::string>("footer!")),
                  ElementsAre(VariantWith<std::string>("how"),
                              VariantWith<StringLiteral>(StringLiteral("are")),
                              VariantWith<std::string>("you?"))));
}

TEST(SimpleIOTest, UsingAllAddLineVariationsDoNotInteractPoorly) {
  SimpleIO s;
  s.AddFooterLine("footer").AddHeaderLine("header").AddLine("line");

  EXPECT_THAT(s.LinesInFooter(),
              ElementsAre(ElementsAre(VariantWith<std::string>("footer"))));
  EXPECT_THAT(s.LinesInHeader(),
              ElementsAre(ElementsAre(VariantWith<std::string>("header"))));
  EXPECT_THAT(s.LinesPerTestCase(),
              ElementsAre(ElementsAre(VariantWith<std::string>("line"))));
}

// -----------------------------------------------------------------------------
//  SimpleIOExporter

TEST(SimpleIOExporterTest, ExportLinesWorksAsExpected) {
  moriarty_internal::VariableSet variable_set;
  ABSL_CHECK_OK(variable_set.AddVariable("N", MInteger()));

  std::vector<moriarty_internal::ValueSet> value_sets(3);
  value_sets[0].Set<MInteger>("N", 10);
  value_sets[1].Set<MInteger>("N", 20);
  value_sets[2].Set<MInteger>("N", 30);

  std::stringstream ss;
  SimpleIOExporter exporter = SimpleIO().AddLine("N").Exporter(ss);

  moriarty_internal::ExporterManager(&exporter).SetGeneralConstraints(
      variable_set);
  moriarty_internal::ExporterManager(&exporter).SetAllValues(value_sets);
  moriarty_internal::ExporterManager(&exporter).SetTestCaseMetadata(
      std::vector<TestCaseMetadata>(3));

  {
    exporter.ExportTestCases();
    EXPECT_THAT(ss.str(), StrEq("10\n20\n30\n"));
  }
}

TEST(SimpleIOExporterTest, ExportHeaderAndFooterLinesWorksAsExpected) {
  moriarty_internal::VariableSet variable_set;
  ABSL_CHECK_OK(variable_set.AddVariable("a", MInteger()));
  ABSL_CHECK_OK(variable_set.AddVariable("b", MInteger()));
  ABSL_CHECK_OK(variable_set.AddVariable("c", MInteger()));

  std::vector<moriarty_internal::ValueSet> value_sets(3);
  value_sets[0].Set<MInteger>("a", 10);
  value_sets[0].Set<MInteger>("b", 20);
  value_sets[0].Set<MInteger>("c", 30);

  value_sets[1].Set<MInteger>("a", 11);
  value_sets[1].Set<MInteger>("b", 21);
  value_sets[1].Set<MInteger>("c", 31);

  value_sets[2].Set<MInteger>("a", 12);
  value_sets[2].Set<MInteger>("b", 22);
  value_sets[2].Set<MInteger>("c", 32);

  std::stringstream ss;
  SimpleIOExporter exporter = SimpleIO()
                                  .AddHeaderLine(StringLiteral("start"))
                                  .AddLine(StringLiteral("line"), "a", "b", "c")
                                  .AddFooterLine(StringLiteral("end"))
                                  .Exporter(ss);

  moriarty_internal::ExporterManager(&exporter).SetGeneralConstraints(
      variable_set);
  moriarty_internal::ExporterManager(&exporter).SetAllValues(value_sets);
  moriarty_internal::ExporterManager(&exporter).SetTestCaseMetadata(
      std::vector<TestCaseMetadata>(3));

  {
    exporter.ExportTestCases();
    EXPECT_THAT(ss.str(), StrEq(R"(start
line 10 20 30
line 11 21 31
line 12 22 32
end
)"));
  }
}

TEST(SimpleIOExporterTest, ExportWithNumberOfTestCasesPrintsCases) {
  moriarty_internal::VariableSet variable_set;
  ABSL_CHECK_OK(variable_set.AddVariable("a", MInteger()));
  ABSL_CHECK_OK(variable_set.AddVariable("b", MInteger()));
  ABSL_CHECK_OK(variable_set.AddVariable("c", MInteger()));

  std::vector<moriarty_internal::ValueSet> value_sets(2);
  value_sets[0].Set<MInteger>("a", 10);
  value_sets[0].Set<MInteger>("b", 20);
  value_sets[0].Set<MInteger>("c", 30);

  value_sets[1].Set<MInteger>("a", 11);
  value_sets[1].Set<MInteger>("b", 21);
  value_sets[1].Set<MInteger>("c", 31);

  std::stringstream ss;
  SimpleIOExporter exporter = SimpleIO()
                                  .WithNumberOfTestCasesInHeader()
                                  .AddLine("a", "b", "c")
                                  .Exporter(ss);

  moriarty_internal::ExporterManager(&exporter).SetGeneralConstraints(
      variable_set);
  moriarty_internal::ExporterManager(&exporter).SetAllValues(value_sets);
  moriarty_internal::ExporterManager(&exporter).SetTestCaseMetadata(
      std::vector<TestCaseMetadata>(2));  // 2 = number of test cases

  {
    exporter.ExportTestCases();
    EXPECT_THAT(ss.str(), StrEq(R"(2
10 20 30
11 21 31
)"));
  }
}

// -----------------------------------------------------------------------------
//  SimpleIOImporter

TEST(SimpleIOImporterTest, ImportLinesWorksAsExpected) {
  using Case = ExampleTestCase;

  moriarty_internal::VariableSet variable_set;
  ABSL_CHECK_OK(variable_set.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variable_set.AddVariable("S", MInteger()));

  std::stringstream ss("1 11\n2 22\n3 33\n4 44\n");
  SimpleIOImporter importer = SimpleIO().AddLine("R", "S").Importer(ss);
  importer.SetNumTestCases(4);

  moriarty_internal::ImporterManager(&importer).SetGeneralConstraints(
      variable_set);

  MORIARTY_ASSERT_OK(importer.ImportTestCases());
  EXPECT_THAT(GetExportedCases<TwoIntegerExporter>(
                  moriarty_internal::ImporterManager(&importer).GetTestCases()),
              ElementsAre(Case({.r = 1, .s = 11}), Case({.r = 2, .s = 22}),
                          Case({.r = 3, .s = 33}), Case({.r = 4, .s = 44})));
}

TEST(SimpleIOImporterTest, ExportHeaderAndFooterLinesWorksAsExpected) {
  using Case = ExampleTestCase;

  moriarty_internal::VariableSet variable_set;
  ABSL_CHECK_OK(variable_set.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variable_set.AddVariable("S", MInteger()));

  std::stringstream ss(R"(hello
1 XX
11
2 XX
22
3 XX
33
4 XX
44
end
)");
  SimpleIOImporter importer = SimpleIO()
                                  .AddHeaderLine(StringLiteral("hello"))
                                  .AddLine("R", StringLiteral("XX"))
                                  .AddLine("S")
                                  .AddFooterLine(StringLiteral("end"))
                                  .Importer(ss);
  importer.SetNumTestCases(4);

  moriarty_internal::ImporterManager(&importer).SetGeneralConstraints(
      variable_set);

  MORIARTY_ASSERT_OK(importer.ImportTestCases());
  EXPECT_THAT(GetExportedCases<TwoIntegerExporter>(
                  moriarty_internal::ImporterManager(&importer).GetTestCases()),
              ElementsAre(Case({.r = 1, .s = 11}), Case({.r = 2, .s = 22}),
                          Case({.r = 3, .s = 33}), Case({.r = 4, .s = 44})));
}

TEST(SimpleIOImporterTest, ImportWrongTokenFails) {
  std::stringstream ss("these are wrong words");
  SimpleIOImporter importer =
      SimpleIO()
          .AddLine(StringLiteral("these"), StringLiteral("are"),
                   StringLiteral("right"), StringLiteral("words"))
          .Importer(ss);

  EXPECT_THAT(importer.ImportTestCases(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Expected to read 'right'")));
}

TEST(SimpleIOImporterTest, ImportWrongWhitespaceFails) {
  moriarty_internal::VariableSet variable_set;
  MORIARTY_ASSERT_OK(variable_set.AddVariable("R", MInteger()));
  MORIARTY_ASSERT_OK(variable_set.AddVariable("S", MInteger()));

  std::stringstream ss("1\t11\n");
  SimpleIOImporter importer = SimpleIO().AddLine("R", "S").Importer(ss);
  importer.SetNumTestCases(1);

  moriarty_internal::ImporterManager(&importer).SetGeneralConstraints(
      variable_set);

  EXPECT_THAT(importer.ImportTestCases(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Expected ' ', but got '\\t'")));
}

TEST(SimpleIOImporterTest, ImportWithNumberOfTestCasesInHeaderWorksAsExpected) {
  using Case = ExampleTestCase;

  moriarty_internal::VariableSet variable_set;
  ABSL_CHECK_OK(variable_set.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variable_set.AddVariable("S", MInteger()));

  std::stringstream ss("4\n1 11\n2 22\n3 33\n4 44\n");
  SimpleIOImporter importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer(ss);

  moriarty_internal::ImporterManager(&importer).SetGeneralConstraints(
      variable_set);
  MORIARTY_EXPECT_OK(importer.ImportTestCases());
  EXPECT_THAT(GetExportedCases<TwoIntegerExporter>(
                  moriarty_internal::ImporterManager(&importer).GetTestCases()),
              ElementsAre(Case({.r = 1, .s = 11}), Case({.r = 2, .s = 22}),
                          Case({.r = 3, .s = 33}), Case({.r = 4, .s = 44})));
}

TEST(SimpleIOImporterTest,
     ImportWithNumberOfTestCasesInHeaderFailsOnTooHighNumberOfCases) {
  moriarty_internal::VariableSet variable_set;
  ABSL_CHECK_OK(variable_set.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variable_set.AddVariable("S", MInteger()));

  std::stringstream ss("6\n1 11\n2 22\n3 33\n4 44\n");
  SimpleIOImporter importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer(ss);

  moriarty_internal::ImporterManager(&importer).SetGeneralConstraints(
      variable_set);

  EXPECT_THAT(importer.ImportTestCases(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("but got EOF")));
}

TEST(SimpleIOImporterTest,
     ImportWithNumberOfTestCasesInHeaderFailsOnNegativeNumberOfCases) {
  moriarty_internal::VariableSet variable_set;
  ABSL_CHECK_OK(variable_set.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variable_set.AddVariable("S", MInteger()));

  std::stringstream ss("-44\n1 11\n2 22\n3 33\n4 44\n");
  SimpleIOImporter importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer(ss);

  moriarty_internal::ImporterManager(&importer).SetGeneralConstraints(
      variable_set);

  EXPECT_THAT(importer.ImportTestCases(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Number of test cases must be nonnegative")));
}

TEST(SimpleIOImporterTest,
     ImportWithNumberOfTestCasesInHeaderFailsOnNonInteger) {
  moriarty_internal::VariableSet variable_set;
  ABSL_CHECK_OK(variable_set.AddVariable("R", MInteger()));
  ABSL_CHECK_OK(variable_set.AddVariable("S", MInteger()));

  std::stringstream ss("hello\n1 11\n2 22\n3 33\n4 44\n");
  SimpleIOImporter importer =
      SimpleIO().WithNumberOfTestCasesInHeader().AddLine("R", "S").Importer(ss);

  moriarty_internal::ImporterManager(&importer).SetGeneralConstraints(
      variable_set);

  EXPECT_THAT(importer.ImportTestCases(),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Unable to parse number of test cases")));
}

}  // namespace
}  // namespace moriarty
