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

#include "src/exporter.h"

#include <cstdint>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/algorithm/container.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/test_case.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"

namespace moriarty {
namespace {

using ::moriarty_testing::IsMisconfigured;
using ::moriarty_testing::IsValueNotFound;
using ::moriarty_testing::IsVariableNotFound;
using ::testing::Each;
using ::testing::ElementsAre;
using ::testing::IsNull;
using ::testing::Pointee;
using ::testing::Property;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

// ProtectedExporter
//
// Class (for tests only) that promotes all protected methods to public so they
// may be tested.
class ProtectedExporter : public Exporter {
 public:
  void ExportTestCase() override {}  // Do nothing.

  using Exporter::GetValue;
  using Exporter::NumTestCases;
  using Exporter::SetIOConfig;
  using Exporter::TryGetTestCaseMetadata;
  using Exporter::TryGetValue;
};

// The core functions that are override'd. We need these since we make
// guarantees about what happens in each of these when `ExportTestCases()` is
// called.
enum class ExporterFn {
  kStartExport,
  kExportTestCase,
  kTestCaseDivider,
  kEndExport
};

// ExporterMetadata
//
// Keeps track of how many times each function is called, and what the
// TestCaseMetadata was at each step.
class ExporterMetadata : public Exporter {
 public:
  void StartExport() override { fn_calls_.push_back(ExporterFn::kStartExport); }
  void ExportTestCase() override {
    fn_calls_.push_back(ExporterFn::kExportTestCase);
    test_case_metadata_in_export_test_case_.push_back(GetTestCaseMetadata());
  }
  void TestCaseDivider() override {
    fn_calls_.push_back(ExporterFn::kTestCaseDivider);
  }
  void EndExport() override { fn_calls_.push_back(ExporterFn::kEndExport); }

  std::vector<TestCaseMetadata> GetTestCaseMetadataInExportTestCase() const {
    return test_case_metadata_in_export_test_case_;
  }
  std::vector<TestCaseMetadata> GetTestCaseMetadataInTestCaseDivider() const {
    return test_case_metadata_in_test_case_divider_;
  }

  // Which function was called in which order.
  std::vector<ExporterFn> FunctionCallOrder() const { return fn_calls_; }

 private:
  std::vector<TestCaseMetadata> test_case_metadata_in_export_test_case_;
  std::vector<TestCaseMetadata> test_case_metadata_in_test_case_divider_;

  std::vector<ExporterFn> fn_calls_;
};

struct ListOfValueSetAndMetadata {
  std::vector<moriarty_internal::ValueSet> values;
  std::vector<TestCaseMetadata> metadata;
};

// Creates several test cases, each with the variable "A" set to the
// corresponding found in `A_values`.
ListOfValueSetAndMetadata GetValueSetAndMetadata(
    std::vector<int64_t> A_values) {
  ListOfValueSetAndMetadata result;
  for (int i = 0; i < A_values.size(); i++) {
    {
      moriarty_internal::ValueSet vals;
      vals.Set<MInteger>("A", A_values[i]);
      result.values.push_back(vals);
    }
    {
      TestCaseMetadata meta;
      meta.SetTestCaseNumber(i + 1);
      result.metadata.push_back(meta);
    }
  }
  return result;
}

// Helper function that creates and seeds an exporter with `num_test_cases`.
// Each case has a single variable (MInteger) set named "A".
// The value of "A" should not be depended on. If you depend on a specific
// value, seed it yourself for readability of the test.
template <typename T>
T GetSeededExporter(int num_test_cases) {
  moriarty_internal::VariableSet variables;
  ABSL_CHECK_OK(variables.AddVariable("A", MInteger()));

  std::vector<int64_t> A_values(num_test_cases);
  absl::c_iota(A_values, 31415);  // Do not depend on 31415.
  auto [values, metadata] = GetValueSetAndMetadata(A_values);

  T exporter;
  moriarty_internal::ExporterManager(&exporter).SetGeneralConstraints(
      variables);
  moriarty_internal::ExporterManager(&exporter).SetAllValues(values);
  moriarty_internal::ExporterManager(&exporter).SetTestCaseMetadata(metadata);

  return exporter;
}

// -----------------------------------------------------------------------------
//  Start of tests

TEST(ExporterTest, ExportTestCasesCallsFunctionsInAppropriateOrder) {
  ExporterMetadata exporter =
      GetSeededExporter<ExporterMetadata>(/* num_test_cases = */ 3);

  exporter.ExportTestCases();

  // Note that TestCaseDivider() is not called after the last ExportTestCase()
  EXPECT_THAT(
      exporter.FunctionCallOrder(),
      ElementsAre(ExporterFn::kStartExport, ExporterFn::kExportTestCase,
                  ExporterFn::kTestCaseDivider, ExporterFn::kExportTestCase,
                  ExporterFn::kTestCaseDivider, ExporterFn::kExportTestCase,
                  ExporterFn::kEndExport));
}

TEST(ExporterTest, ExportTestCasesWithNoTestCasesSucceeds) {
  ExporterMetadata exporter =
      GetSeededExporter<ExporterMetadata>(/* num_test_cases = */ 0);

  exporter.ExportTestCases();

  EXPECT_THAT(exporter.FunctionCallOrder(),
              ElementsAre(ExporterFn::kStartExport, ExporterFn::kEndExport));
}

TEST(ExporterTest, ExportTestCasesPassesCorrectMetadataToEachExportTestCase) {
  ExporterMetadata exporter =
      GetSeededExporter<ExporterMetadata>(/* num_test_cases = */ 3);
  moriarty_internal::ExporterManager(&exporter).SetTestCaseMetadata(
      {TestCaseMetadata().SetTestCaseNumber(123),
       TestCaseMetadata().SetTestCaseNumber(456),
       TestCaseMetadata().SetTestCaseNumber(789)});

  exporter.ExportTestCases();

  EXPECT_THAT(exporter.GetTestCaseMetadataInExportTestCase(),
              ElementsAre(Property(&TestCaseMetadata::GetTestCaseNumber, 123),
                          Property(&TestCaseMetadata::GetTestCaseNumber, 456),
                          Property(&TestCaseMetadata::GetTestCaseNumber, 789)));
}

TEST(ExporterDeathTest, TestCaseMetadatasSizeMustBeTheSameAsSetAllValues) {
  {
    ProtectedExporter exporter;
    EXPECT_DEATH(
        {
          moriarty_internal::ExporterManager(&exporter).SetTestCaseMetadata(
              std::vector<TestCaseMetadata>(3));  // Size = 3, with no Values
        },
        "metadata.size()");
  }

  {
    ProtectedExporter exporter;
    moriarty_internal::ExporterManager(&exporter).SetAllValues(
        std::vector<moriarty_internal::ValueSet>(4));  // Size = 4
    EXPECT_DEATH(
        {
          moriarty_internal::ExporterManager(&exporter).SetTestCaseMetadata(
              std::vector<TestCaseMetadata>(3));  // Size = 3
        },
        "metadata.size()");
  }
}

TEST(ExporterTest, TryGetTestCaseMetadataWithNoMetadataFails) {
  EXPECT_THAT(ProtectedExporter().TryGetTestCaseMetadata(),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(ExporterTest, ByDefaultNumTestCasesReturnsZero) {
  EXPECT_EQ(ProtectedExporter().NumTestCases(), 0);
}

TEST(ExporterTest, NumTestCasesReturnsNumberOfValuesSet) {
  ProtectedExporter exporter;
  std::vector<moriarty_internal::ValueSet> values(5);
  moriarty_internal::ExporterManager(&exporter).SetAllValues(values);

  EXPECT_EQ(exporter.NumTestCases(), 5);
}

TEST(ExporterTest, GetAbstractVariableExtractsVariablesCorrectly) {
  ProtectedExporter exporter;
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MInteger()));
  moriarty_internal::ExporterManager(&exporter).SetGeneralConstraints(
      variables);

  EXPECT_THAT(
      moriarty_internal::ExporterManager(&exporter).GetAbstractVariable("A"),
      IsOkAndHolds(Pointee(Property(
          &moriarty_internal::AbstractVariable::Typename, "MInteger"))));
  EXPECT_THAT(
      moriarty_internal::ExporterManager(&exporter).GetAbstractVariable("B"),
      IsVariableNotFound("B"));
}

TEST(ExporterTest, GetAbstractVariableWithNoGeneralConstraintsReturnsNotFound) {
  ProtectedExporter exporter;

  EXPECT_THAT(
      moriarty_internal::ExporterManager(&exporter).GetAbstractVariable("A"),
      IsVariableNotFound("A"));
}

// Exporter which calls `TryGetValue("A")` and `TryGetValue("B")` and
// stores the result of all calls. The results can be extracted via
// `GetCalls()`.
//
// Only "A" exists in each test case and those values are provided in the
// constructor, one per test case.
class GetValuesExporter : public Exporter {
 public:
  GetValuesExporter(std::vector<int64_t> A_values,
                    ExporterFn fn_to_call_get_variable)
      : fn_to_call_get_variable_(fn_to_call_get_variable) {
    auto [values, metadata] = GetValueSetAndMetadata(A_values);
    moriarty_internal::ExporterManager(this).SetAllValues(values);
    moriarty_internal::ExporterManager(this).SetTestCaseMetadata(metadata);
  }

  void StartExport() override {
    if (fn_to_call_get_variable_ == ExporterFn::kStartExport) CallGetValue();
  }
  void ExportTestCase() override {
    if (fn_to_call_get_variable_ == ExporterFn::kExportTestCase) CallGetValue();
  }
  void TestCaseDivider() override {
    if (fn_to_call_get_variable_ == ExporterFn::kTestCaseDivider)
      CallGetValue();
  }
  void EndExport() override {
    if (fn_to_call_get_variable_ == ExporterFn::kEndExport) CallGetValue();
  }

  const std::vector<absl::StatusOr<int64_t>>& ResultsOfCallsToGetValueA()
      const {
    return results_a_;
  }

  const std::vector<absl::StatusOr<int64_t>>& ResultsOfCallsToGetValueB()
      const {
    return results_b_;
  }

 private:
  ExporterFn fn_to_call_get_variable_;
  std::vector<absl::StatusOr<int64_t>> results_a_;
  std::vector<absl::StatusOr<int64_t>> results_b_;

  void CallGetValue() {
    results_a_.push_back(TryGetValue<moriarty::MInteger>("A"));
    if (results_a_.back().ok()) {
      ABSL_CHECK_EQ(*results_a_.back(), GetValue<moriarty::MInteger>("A"));
    }

    // Does not exist.
    results_b_.push_back(TryGetValue<moriarty::MInteger>("B"));
  }
};

TEST(ExporterTest, GetValueExtractsVariablesCorrectlyInExportTestCase) {
  GetValuesExporter exporter({12, 34, 56}, ExporterFn::kExportTestCase);

  exporter.ExportTestCases();

  EXPECT_THAT(
      exporter.ResultsOfCallsToGetValueA(),
      ElementsAre(IsOkAndHolds(12), IsOkAndHolds(34), IsOkAndHolds(56)));
}

TEST(ExporterTest,
     GetValueOnNonExistentVariableFailsAsIntendedInExportTestCase) {
  GetValuesExporter exporter({12, 34, 56}, ExporterFn::kExportTestCase);

  exporter.ExportTestCases();

  EXPECT_THAT(exporter.ResultsOfCallsToGetValueB(),
              ElementsAre(IsValueNotFound("B"), IsValueNotFound("B"),
                          IsValueNotFound("B")));
}

TEST(ExporterTest, GetValueCannotBeCalledInOtherOverridedFunctions) {
  {
    GetValuesExporter exporter({12, 34, 56}, ExporterFn::kStartExport);
    exporter.ExportTestCases();
    EXPECT_THAT(exporter.ResultsOfCallsToGetValueA(),
                Each(IsMisconfigured(InternalConfigurationType::kValueSet)));
    EXPECT_THAT(exporter.ResultsOfCallsToGetValueB(),
                Each(IsMisconfigured(InternalConfigurationType::kValueSet)));
  }
  {
    GetValuesExporter exporter({12, 34, 56}, ExporterFn::kTestCaseDivider);
    exporter.ExportTestCases();
    EXPECT_THAT(exporter.ResultsOfCallsToGetValueA(),
                Each(IsMisconfigured(InternalConfigurationType::kValueSet)));
    EXPECT_THAT(exporter.ResultsOfCallsToGetValueB(),
                Each(IsMisconfigured(InternalConfigurationType::kValueSet)));
  }
  {
    GetValuesExporter exporter({12, 34, 56}, ExporterFn::kEndExport);
    exporter.ExportTestCases();
    EXPECT_THAT(exporter.ResultsOfCallsToGetValueA(),
                Each(IsMisconfigured(InternalConfigurationType::kValueSet)));
    EXPECT_THAT(exporter.ResultsOfCallsToGetValueB(),
                Each(IsMisconfigured(InternalConfigurationType::kValueSet)));
  }
}

TEST(ExporterTest, GetIOConfigIsNullByDefault) {
  ProtectedExporter exporter;
  EXPECT_THAT(moriarty_internal::ExporterManager(&exporter).GetIOConfig(),
              IsNull());
}

TEST(ExporterTest, SetIOConfigUpdatesGetIOConfig) {
  ProtectedExporter exporter;
  librarian::IOConfig io_config;

  exporter.SetIOConfig(&io_config);

  // Checking the memory is identical.
  EXPECT_EQ(moriarty_internal::ExporterManager(&exporter).GetIOConfig(),
            &io_config);
}

TEST(ExporterTest, ExportingZeroCasesIsFine) {
  class NoOpExporter : public Exporter {
   public:
    void ExportTestCase() override {}  // Do nothing
  };

  // Ensuring the following line doesn't crash.
  NoOpExporter().ExportTestCases();
}

TEST(ExporterTest, DoingNothingInExporterCallsDefaultStartExportEndExport) {
  class NoOpExporter : public Exporter {
   public:
    void ExportTestCase() override {}  // Do nothing
  };

  NoOpExporter exporter =
      GetSeededExporter<NoOpExporter>(/* num_test_cases = */ 3);

  // Ensuring the following line doesn't crash or have any restrictions on them.
  exporter.ExportTestCases();
}

}  // namespace
}  // namespace moriarty
