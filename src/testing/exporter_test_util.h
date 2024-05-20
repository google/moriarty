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

#ifndef MORIARTY_SRC_TESTING_EXPORTER_TEST_UTIL_H_
#define MORIARTY_SRC_TESTING_EXPORTER_TEST_UTIL_H_

#include <ostream>
#include <string>
#include <tuple>
#include <vector>

#include "absl/status/statusor.h"
#include "src/exporter.h"
#include "src/internal/value_set.h"
#include "src/test_case.h"
#include "src/testing/mtest_type.h"
#include "src/util/status_macro/status_macros.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty_testing {

// LINT.IfChange

// A convenience shared struct for the following exporters. Each exporter will
// export one or more of these variables.
struct ExampleTestCase {
  // Single integer cases
  int n = 0;
  // Two integer cases
  int r = 0;
  int s = 0;
  // Two fake cases
  TestType x = 0;
  TestType y = 0;
  // Strings
  std::string str = "";

  // Between cases, I sometimes add this...
  std::string p;

  // Metadata...
  moriarty::TestCaseMetadata metadata;

  // Equality does not consider metadata
  bool operator==(const ExampleTestCase& other) const {
    return std::tie(n, x, y) == std::tie(other.n, other.x, other.y);
  }
};

// To get pretty error messages when tests fail
inline std::ostream& operator<<(std::ostream& os, const ExampleTestCase& c) {
  os << "{ n=" << c.n << " x=" << c.x << " y=" << c.y << " }";
  return os;
}

// Generates one case with an integer N = 0
struct SingleIntegerExporter : public moriarty::Exporter {
  explicit SingleIntegerExporter(std::vector<ExampleTestCase>* cases)
      : test_cases(*cases) {}

  void ExportTestCase() override {
    int N = GetValue<moriarty::MInteger>("N");

    moriarty::TestCaseMetadata metadata = GetTestCaseMetadata();

    test_cases.push_back({.n = N, .metadata = metadata});
  }

  std::vector<ExampleTestCase>& test_cases;  // Not owned by this class
};

// Generates one case with an integer N = 0
struct SingleStringExporter : public moriarty::Exporter {
  explicit SingleStringExporter(std::vector<ExampleTestCase>* cases)
      : test_cases(*cases) {}

  void ExportTestCase() override {
    std::string str = GetValue<moriarty::MString>("str");

    moriarty::TestCaseMetadata metadata = GetTestCaseMetadata();

    test_cases.push_back({.str = str, .metadata = metadata});
  }

  std::vector<ExampleTestCase>& test_cases;  // Not owned by this class
};

struct SimpleTestTypeExporter : public moriarty::Exporter {
  explicit SimpleTestTypeExporter(std::vector<ExampleTestCase>* cases)
      : test_cases(*cases) {}

  void ExportTestCase() override {
    TestType X = GetValue<MTestType>("X");
    TestType Y = GetValue<MTestType>("Y");

    moriarty::TestCaseMetadata metadata = GetTestCaseMetadata();

    test_cases.push_back({.x = X, .y = Y, .metadata = metadata});
  }

  std::vector<ExampleTestCase>& test_cases;  // Not owned by this class
};

using TwoTestTypeExporter = SimpleTestTypeExporter;

// Same as TwoTestTypeExporter, but also adds things in StartExport,
// EndExport and Divider.
struct TwoTestTypeWithPaddingExporter : public moriarty::Exporter {
  explicit TwoTestTypeWithPaddingExporter(std::vector<ExampleTestCase>* cases)
      : test_cases(*cases) {}

  void StartExport() override { test_cases.push_back({.p = "Start"}); }

  void TestCaseDivider() override { test_cases.push_back({.p = "Divider"}); }

  void EndExport() override { test_cases.push_back({.p = "End"}); }

  void ExportTestCase() override {
    TestType X = GetValue<MTestType>("X");
    TestType Y = GetValue<MTestType>("Y");

    moriarty::TestCaseMetadata metadata = GetTestCaseMetadata();

    test_cases.push_back({.x = X, .y = Y, .metadata = metadata});
  }

  std::vector<ExampleTestCase>& test_cases;  // Not owned by this class
};

struct TwoIntegerExporter : public moriarty::Exporter {
  explicit TwoIntegerExporter(std::vector<ExampleTestCase>* cases)
      : test_cases(*cases) {}

  void ExportTestCase() override {
    int R = GetValue<moriarty::MInteger>("R");
    int S = GetValue<moriarty::MInteger>("S");

    moriarty::TestCaseMetadata metadata = GetTestCaseMetadata();

    test_cases.push_back({.r = R, .s = S, .metadata = metadata});
  }

  std::vector<ExampleTestCase>& test_cases;  // Not owned by this class
};

struct TwoTestTypeWrongTypeExporter : public moriarty::Exporter {
  explicit TwoTestTypeWrongTypeExporter(std::vector<ExampleTestCase>* cases)
      : test_cases(*cases) {}

  void ExportTestCase() override {
    TestType R = GetValue<MTestType>("R");
    TestType S = GetValue<MTestType>("S");

    moriarty::TestCaseMetadata metadata = GetTestCaseMetadata();

    test_cases.push_back({.r = R, .s = S, .metadata = metadata});
  }

  std::vector<ExampleTestCase>& test_cases;  // Not owned by this class
};

// LINT.ThenChange(generator_test_util.h)

// Helper functions to get test cases exported.
inline moriarty::TestCaseMetadata GenericMetadataForTests() {
  return moriarty::TestCaseMetadata()
      .SetTestCaseNumber(12)
      .SetGeneratorMetadata({.generator_name = "test",
                             .generator_iteration = 34,
                             .case_number_in_generator = 56});
}

template <typename ExporterType>
std::vector<ExampleTestCase> GetExportedCases(
    const std::vector<moriarty::moriarty_internal::ValueSet>& test_cases) {
  std::vector<ExampleTestCase> result;
  ExporterType exporter(&result);

  moriarty::moriarty_internal::ExporterManager(&exporter).SetAllValues(
      test_cases);

  // All metadata is the same for each case here.
  moriarty::moriarty_internal::ExporterManager(&exporter).SetTestCaseMetadata(
      std::vector<moriarty::TestCaseMetadata>(test_cases.size(),
                                              GenericMetadataForTests()));

  exporter.ExportTestCases();
  return result;
}

template <typename ExporterType, typename GeneratorType>
absl::StatusOr<std::vector<ExampleTestCase>> GetGeneratedCasesViaExporter(
    GeneratorType generator) {
  MORIARTY_ASSIGN_OR_RETURN(
      std::vector<moriarty::moriarty_internal::ValueSet> test_cases,
      RunGenerateForTest(generator));

  return GetExportedCases<ExporterType>(test_cases);
}

}  // namespace moriarty_testing

#endif  // MORIARTY_SRC_TESTING_EXPORTER_TEST_UTIL_H_
