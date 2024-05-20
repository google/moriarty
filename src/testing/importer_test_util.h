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

#ifndef MORIARTY_SRC_TESTING_IMPORTER_TEST_UTIL_H_
#define MORIARTY_SRC_TESTING_IMPORTER_TEST_UTIL_H_

#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "src/importer.h"
#include "src/variables/minteger.h"

namespace moriarty_testing {

// Keeps track of how many times each of the 4 importer functions are called and
// CHECKs each is valid.
struct ImportFunctionCounter {
  // Information to track how many times each function is called to ensure
  // internal monitoring.
  int header_cnt = 0;
  int test_case_cnt = 0;
  int between_cnt = 0;
  int footer_cnt = 0;

  void Check(int header_count, int test_case_count, int between_count,
             int footer_count) {
    ABSL_CHECK_EQ(header_cnt, header_count);
    ABSL_CHECK_EQ(test_case_cnt, test_case_count);
    ABSL_CHECK_EQ(between_cnt, between_count);
    ABSL_CHECK_EQ(footer_cnt, footer_count);
  }

  void Header() {
    Check(0, 0, 0, 0);
    header_cnt++;
  }

  void TestCase(int tc) {
    Check(1, tc - 1, tc - 1, 0);
    test_case_cnt++;
  }

  void Between(int tc) {
    Check(1, tc, tc - 1, 0);
    between_cnt++;
  }

  void Footer(bool known_num_test_cases) {
    int num_test_cases = test_case_cnt;
    int num_between =
        (known_num_test_cases ? num_test_cases - 1 : num_test_cases);
    Check(1, num_test_cases, num_between, 0);
    footer_cnt++;
  }
};

// Imports cases with N. Knows how many cases there are.
struct SingleIntegerFromVectorImporter : public moriarty::Importer {
  explicit SingleIntegerFromVectorImporter(std::vector<int> cases)
      : test_cases(std::move(cases)) {}

  absl::Status StartImport() override {
    stats.Header();
    SetNumTestCases(test_cases.size());
    return absl::OkStatus();
  }

  absl::Status ImportTestCase() override {
    stats.TestCase(GetTestCaseMetadata().test_case_number);
    SetValue<moriarty::MInteger>(
        "N", test_cases[GetTestCaseMetadata().test_case_number - 1]);
    return absl::OkStatus();
  }

  absl::Status TestCaseDivider() override {
    stats.Between(GetTestCaseMetadata().test_case_number);
    return absl::OkStatus();
  }

  absl::Status EndImport() override {
    stats.Footer(/* known_num_test_cases = */ true);
    return absl::OkStatus();
  }

  std::vector<int> test_cases;
  ImportFunctionCounter stats;
};

// Imports cases with X and Y. Acts like it does not know how many cases there
// are.
struct TwoVariableFromVectorImporter : public moriarty::Importer {
  explicit TwoVariableFromVectorImporter(std::vector<std::pair<int, int>> cases)
      : test_cases(std::move(cases)), it(test_cases.begin()) {}

  absl::Status StartImport() override {
    stats.Header();
    return absl::OkStatus();
  }

  absl::Status ImportTestCase() override {
    stats.TestCase(GetTestCaseMetadata().test_case_number);

    SetValue<moriarty::MInteger>("R", it->first);
    SetValue<moriarty::MInteger>("S", it->second);
    return absl::OkStatus();
  }

  absl::Status TestCaseDivider() override {
    stats.Between(GetTestCaseMetadata().test_case_number);

    ++it;
    if (it == test_cases.end()) Done();
    return absl::OkStatus();
  }

  absl::Status EndImport() override {
    stats.Footer(/* known_num_test_cases = */ false);
    return absl::OkStatus();
  }

  std::vector<std::pair<int, int>> test_cases;
  std::vector<std::pair<int, int>>::iterator it;  // The current case.

  ImportFunctionCounter stats;
};

}  // namespace moriarty_testing

#endif  // MORIARTY_SRC_TESTING_IMPORTER_TEST_UTIL_H_
