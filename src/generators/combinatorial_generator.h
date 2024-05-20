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

#ifndef MORIARTY_SRC_GENERATORS_COMBINATORIAL_GENERATOR_H_
#define MORIARTY_SRC_GENERATORS_COMBINATORIAL_GENERATOR_H_

// CombinatorialCoverage is a generator that attempts to smartly combine
// difficult cases from each variable to make a large set of hard test cases.
//
// Example Usage:
//
// Moriarty()
//    .AddVariable("N", MInteger().Between(1, 10))
//    .AddVariable("A", MArray<MInteger>().OfLength("N"))
//    .AddGenerator(CombinatorialCoverage());  // Will generate tricky cases.

#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"
#include "src/generator.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/combinatorial_coverage.h"

namespace moriarty {

struct InitializeCasesInfo {
  std::vector<std::vector<std::unique_ptr<moriarty_internal::AbstractVariable>>>
      cases;
  std::vector<std::string> variable_names;
  std::vector<int> dimension_sizes;
};

// Generates test cases using covering arrays based on the difficult instances
// of the defined variables.
class CombinatorialCoverage : public moriarty::Generator {
 public:
  void GenerateTestCases() override;

 private:
  // InitializeCases()
  //
  // Uses the map of variables to extract the difficult instances from the
  // abstract variables, the name of the variable and define the dimensions
  // array for the generation of the covering array.
  InitializeCasesInfo InitializeCases(
      const absl::flat_hash_map<
          std::string, std::unique_ptr<moriarty_internal::AbstractVariable>>&
          vars);

  // CreateTestCases()
  //
  // Creates all the test cases defined in the covering array.
  void CreateTestCases(
      absl::Span<const CoveringArrayTestCase> covering_array,
      absl::Span<const std::vector<
          std::unique_ptr<moriarty_internal::AbstractVariable>>>
          cases,
      absl::Span<const std::string> variable_names);
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_GENERATORS_COMBINATORIAL_GENERATOR_H_
