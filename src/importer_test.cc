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

#include "src/importer.h"

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/testing/status_test_util.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace {

using ::moriarty_testing::IsUnsatisfiedConstraint;
using ::moriarty_testing::IsVariableNotFound;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::IsNull;
using ::testing::SizeIs;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;

// ProtectedImporter
//
// Class (for tests only) that promotes all protected methods to public so they
// may be tested.
class ProtectedImporter : public Importer {
 public:
  absl::Status ImportTestCase() override {
    return absl::UnimplementedError(
        "Don't use ProtectedImporter for importing");
  }

  using Importer::Done;
  using Importer::GetTestCaseMetadata;
  using Importer::IsDone;
  using Importer::SatisfiesConstraints;
  using Importer::SetIOConfig;
  using Importer::SetNumTestCases;
  using Importer::SetValue;
};

// ImporterMetadata
//
// Keeps track of how many times each function is called, and what the
// TestCaseMetadata was at each step.
class ImporterMetadata : public Importer {
 public:
  enum class ImportStyle {
    kCallSetNumTestCases,
    kCallDoneInImportTestCase,
    kCallDoneInTestCaseDivider
  };

  ImporterMetadata(ImportStyle import_style, int num_test_cases)
      : import_style_(import_style), num_test_cases_(num_test_cases) {}

  absl::Status StartImport() override {
    fn_calls_.push_back(Function::kStartImport);

    if (import_style_ == ImportStyle::kCallSetNumTestCases)
      SetNumTestCases(num_test_cases_);
    return absl::OkStatus();
  }

  absl::Status ImportTestCase() override {
    fn_calls_.push_back(Function::kImportTestCase);

    if (current_test_case_ == num_test_cases_ &&
        import_style_ == ImportStyle::kCallDoneInImportTestCase) {
      Done();
    }

    TestCaseMetadata metadata = GetTestCaseMetadata();
    test_case_numbers_in_import_test_case_.push_back(metadata.test_case_number);
    num_test_cases_in_import_test_case_.push_back(metadata.num_test_cases);

    return absl::OkStatus();
  }

  absl::Status TestCaseDivider() override {
    fn_calls_.push_back(Function::kTestCaseDivider);

    if (current_test_case_ == num_test_cases_ &&
        import_style_ == ImportStyle::kCallDoneInTestCaseDivider) {
      Done();
    }

    TestCaseMetadata metadata = GetTestCaseMetadata();
    test_case_numbers_in_test_case_divider_.push_back(
        metadata.test_case_number);
    num_test_cases_in_test_case_divider_.push_back(metadata.num_test_cases);

    current_test_case_++;

    return absl::OkStatus();
  }

  absl::Status EndImport() override {
    fn_calls_.push_back(Function::kEndImport);
    return absl::OkStatus();
  }

  const std::vector<int>& GetTestCaseNumbersInImportTestCase() const {
    return test_case_numbers_in_import_test_case_;
  }
  const std::vector<int>& GetNumTestCasesInImportTestCase() const {
    return num_test_cases_in_import_test_case_;
  }
  const std::vector<int>& GetTestCaseNumbersInTestCaseDivider() const {
    return test_case_numbers_in_test_case_divider_;
  }
  const std::vector<int>& GetNumTestCasesInTestCaseDivider() const {
    return num_test_cases_in_test_case_divider_;
  }

  // Which function was called in which order.
  enum class Function {
    kStartImport,
    kImportTestCase,
    kTestCaseDivider,
    kEndImport
  };
  std::vector<Function> FunctionCallOrder() const { return fn_calls_; }

 private:
  ImportStyle import_style_;
  int num_test_cases_;
  int current_test_case_ = 1;

  std::vector<int> test_case_numbers_in_import_test_case_;
  std::vector<int> num_test_cases_in_import_test_case_;
  std::vector<int> test_case_numbers_in_test_case_divider_;
  std::vector<int> num_test_cases_in_test_case_divider_;

  std::vector<Function> fn_calls_;
};

// -----------------------------------------------------------------------------
//  Start of tests

TEST(ImporterTest, ImportingZeroCasesIsFine) {
  class NoOpImporter : public Importer {
   public:
    absl::Status ImportTestCase() override {
      Done();
      return absl::OkStatus();
    }
  };

  NoOpImporter importer;
  MORIARTY_ASSERT_OK(importer.ImportTestCases());

  EXPECT_THAT(moriarty_internal::ImporterManager(&importer).GetTestCases(),
              IsEmpty());
}

TEST(ImporterTest, SetValueShouldSetTheValueInCurrentTestCase) {
  ProtectedImporter importer;
  moriarty_internal::ImporterManager manager(&importer);
  // Including an `MString` here to allow `.SetValue("X", string)` below.
  [[maybe_unused]] MString S;

  importer.SetValue<MInteger>("N", 123);
  importer.SetValue<MString>("X", "hello");

  EXPECT_THAT(manager.GetCurrentTestCase().Get<MInteger>("N"),
              IsOkAndHolds(123));
  EXPECT_THAT(manager.GetCurrentTestCase().Get<MString>("X"),
              IsOkAndHolds("hello"));
}

TEST(ImporterTest, TestCaseMetadataCorrectIfSetNumTestCasesUsed) {
  ImporterMetadata importer(ImporterMetadata::ImportStyle::kCallSetNumTestCases,
                            5);

  MORIARTY_ASSERT_OK(importer.ImportTestCases());

  // TestCaseDivider called one fewer time than ImportTestCase.
  EXPECT_THAT(importer.GetTestCaseNumbersInImportTestCase(),
              ElementsAre(1, 2, 3, 4, 5));
  EXPECT_THAT(importer.GetNumTestCasesInImportTestCase(),
              ElementsAre(5, 5, 5, 5, 5));
  EXPECT_THAT(importer.GetTestCaseNumbersInTestCaseDivider(),
              ElementsAre(1, 2, 3, 4));
  EXPECT_THAT(importer.GetNumTestCasesInTestCaseDivider(),
              ElementsAre(5, 5, 5, 5));
}

TEST(ImporterTest, TestCaseMetadataCorrectIfDoneIsCalledInImportTestCase) {
  ImporterMetadata importer(
      ImporterMetadata::ImportStyle::kCallDoneInImportTestCase, 6);

  MORIARTY_ASSERT_OK(importer.ImportTestCases());

  // TestCaseDivider called one fewer time than ImportTestCase.
  EXPECT_THAT(importer.GetTestCaseNumbersInImportTestCase(),
              ElementsAre(1, 2, 3, 4, 5, 6));
  EXPECT_THAT(importer.GetNumTestCasesInImportTestCase(),
              ElementsAre(0, 0, 0, 0, 0, 0));
  EXPECT_THAT(importer.GetTestCaseNumbersInTestCaseDivider(),
              ElementsAre(1, 2, 3, 4, 5));
  EXPECT_THAT(importer.GetNumTestCasesInTestCaseDivider(),
              ElementsAre(0, 0, 0, 0, 0));
}

TEST(ImporterTest, TestCaseMetadataCorrectIfDoneIsCalledInTestCaseDivider) {
  ImporterMetadata importer(
      ImporterMetadata::ImportStyle::kCallDoneInTestCaseDivider, 7);

  MORIARTY_ASSERT_OK(importer.ImportTestCases());

  // TestCaseDivider called same number of times as ImportTestCase.
  EXPECT_THAT(importer.GetTestCaseNumbersInImportTestCase(),
              ElementsAre(1, 2, 3, 4, 5, 6, 7));
  EXPECT_THAT(importer.GetNumTestCasesInImportTestCase(),
              ElementsAre(0, 0, 0, 0, 0, 0, 0));
  EXPECT_THAT(importer.GetTestCaseNumbersInTestCaseDivider(),
              ElementsAre(1, 2, 3, 4, 5, 6, 7));
  EXPECT_THAT(importer.GetNumTestCasesInTestCaseDivider(),
              ElementsAre(0, 0, 0, 0, 0, 0, 0));
}

TEST(ImporterTest, FunctionsCalledInAppropriateOrderForSetNumTestCases) {
  ImporterMetadata importer(ImporterMetadata::ImportStyle::kCallSetNumTestCases,
                            3);

  MORIARTY_ASSERT_OK(importer.ImportTestCases());

  // Note that TestCaseDivider() is not called after the last ImportTestCase()
  EXPECT_THAT(importer.FunctionCallOrder(),
              ElementsAre(ImporterMetadata::Function::kStartImport,
                          ImporterMetadata::Function::kImportTestCase,
                          ImporterMetadata::Function::kTestCaseDivider,
                          ImporterMetadata::Function::kImportTestCase,
                          ImporterMetadata::Function::kTestCaseDivider,
                          ImporterMetadata::Function::kImportTestCase,
                          ImporterMetadata::Function::kEndImport));
}

TEST(ImporterTest,
     FunctionsCalledInAppropriateOrderWhenDoneCalledInImportTestCase) {
  ImporterMetadata importer(
      ImporterMetadata::ImportStyle::kCallDoneInImportTestCase, 3);

  MORIARTY_ASSERT_OK(importer.ImportTestCases());

  // Note that TestCaseDivider() is not called after the last ImportTestCase()
  EXPECT_THAT(importer.FunctionCallOrder(),
              ElementsAre(ImporterMetadata::Function::kStartImport,
                          ImporterMetadata::Function::kImportTestCase,
                          ImporterMetadata::Function::kTestCaseDivider,
                          ImporterMetadata::Function::kImportTestCase,
                          ImporterMetadata::Function::kTestCaseDivider,
                          ImporterMetadata::Function::kImportTestCase,
                          ImporterMetadata::Function::kEndImport));
}

TEST(ImporterTest,
     FunctionsCalledInAppropriateOrderWhenDoneCalledInTestCaseDivider) {
  ImporterMetadata importer(
      ImporterMetadata::ImportStyle::kCallDoneInTestCaseDivider, 3);

  MORIARTY_ASSERT_OK(importer.ImportTestCases());

  // Note that TestCaseDivider() is called after the last ImportTestCase()
  EXPECT_THAT(importer.FunctionCallOrder(),
              ElementsAre(ImporterMetadata::Function::kStartImport,
                          ImporterMetadata::Function::kImportTestCase,
                          ImporterMetadata::Function::kTestCaseDivider,
                          ImporterMetadata::Function::kImportTestCase,
                          ImporterMetadata::Function::kTestCaseDivider,
                          ImporterMetadata::Function::kImportTestCase,
                          ImporterMetadata::Function::kTestCaseDivider,
                          ImporterMetadata::Function::kEndImport));
}

TEST(ImporterTest, CannotCallBothDoneAndSetNumTestCases) {
  struct BadImporter : Importer {
    absl::Status StartImport() override {
      SetNumTestCases(3);
      return absl::OkStatus();
    }

    absl::Status ImportTestCase() override {
      Done();
      return absl::OkStatus();
    }
  };

  EXPECT_THAT(BadImporter().ImportTestCases(),
              StatusIs(absl::StatusCode::kFailedPrecondition));
}

TEST(ImporterTest, MultipleTestCasesSetsVariablesAppropriartely) {
  struct SimpleImporter : Importer {
    absl::Status StartImport() override {
      SetNumTestCases(3);
      return absl::OkStatus();
    }

    absl::Status ImportTestCase() override {
      SetValue<MInteger>("N", value++);
      SetValue<MInteger>("X", value++);
      return absl::OkStatus();
    }

    int value = 123;
  };
  SimpleImporter importer;

  MORIARTY_ASSERT_OK(importer.ImportTestCases());

  std::vector<moriarty_internal::ValueSet> values =
      moriarty_internal::ImporterManager(&importer).GetTestCases();

  ASSERT_THAT(values, SizeIs(3));

  EXPECT_THAT(values[0].Get<MInteger>("N"), IsOkAndHolds(123));
  EXPECT_THAT(values[0].Get<MInteger>("X"), IsOkAndHolds(124));

  EXPECT_THAT(values[1].Get<MInteger>("N"), IsOkAndHolds(125));
  EXPECT_THAT(values[1].Get<MInteger>("X"), IsOkAndHolds(126));

  EXPECT_THAT(values[2].Get<MInteger>("N"), IsOkAndHolds(127));
  EXPECT_THAT(values[2].Get<MInteger>("X"), IsOkAndHolds(128));
}

TEST(ImporterTest, GetAbstractVariableCanAccessVariablesInGeneralConstraints) {
  ProtectedImporter importer;
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("A", MInteger().Between(1, 10)));

  moriarty_internal::ImporterManager(&importer).SetGeneralConstraints(
      variables);

  EXPECT_THAT(
      moriarty_internal::ImporterManager(&importer).GetAbstractVariable("A"),
      IsOkAndHolds(Not(IsNull())));
  EXPECT_THAT(
      moriarty_internal::ImporterManager(&importer).GetAbstractVariable("B"),
      IsVariableNotFound("B"));
}

TEST(ImporterTest, DoneIsFalseByDefault) {
  EXPECT_FALSE(ProtectedImporter().IsDone());
}

TEST(ImporterTest, CallingDoneShouldUpdateIsDone) {
  ProtectedImporter importer;
  importer.Done();
  EXPECT_TRUE(importer.IsDone());
}

TEST(ImporterTest, GetIOConfigIsNullByDefault) {
  ProtectedImporter importer;
  EXPECT_THAT(moriarty_internal::ImporterManager(&importer).GetIOConfig(),
              IsNull());
}

TEST(ImporterTest, SetIOConfigUpdatesGetIOConfig) {
  ProtectedImporter importer;
  librarian::IOConfig io_config;

  importer.SetIOConfig(&io_config);

  // Checking the memory is identical.
  EXPECT_EQ(moriarty_internal::ImporterManager(&importer).GetIOConfig(),
            &io_config);
}

TEST(ImporterTest, SatisfiesConstraintsWorksBasicCase) {
  MORIARTY_EXPECT_OK(
      ProtectedImporter().SatisfiesConstraints(MInteger().Between(1, 10), 5));
  EXPECT_THAT(
      ProtectedImporter().SatisfiesConstraints(MInteger().Between(1, 10), 15),
      IsUnsatisfiedConstraint("range"));
}

TEST(ImporterTest, SatisfiesConstraintsUsesVariablesFromGeneralConstraints) {
  ProtectedImporter I;
  moriarty_internal::VariableSet variables;
  MORIARTY_ASSERT_OK(variables.AddVariable("N", MInteger().Is(10)));
  moriarty_internal::ImporterManager(&I).SetGeneralConstraints(variables);
  MORIARTY_EXPECT_OK(I.SatisfiesConstraints(MInteger().Between(1, "N"), 5));
  EXPECT_THAT(I.SatisfiesConstraints(MInteger().Between(1, "N"), 11),
              IsUnsatisfiedConstraint("range"));
}

}  // namespace
}  // namespace moriarty
