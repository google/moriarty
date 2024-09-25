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

// Moriarty is a data generation/validation library. Moriarty provides a
// centralized language for data generators to speak in. Moreover, interactions
// between different parameters you are generating is allowed/encouraged.
//
// New data types can be added by subject matter experts and used by everyone
// else.

#ifndef MORIARTY_SRC_MORIARTY_H_
#define MORIARTY_SRC_MORIARTY_H_

#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "src/exporter.h"
#include "src/generator.h"
#include "src/importer.h"
#include "src/internal/status_utils.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/mvariable.h"
#include "src/test_case.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

// Moriarty
//
// This is the central class of the Moriarty suite. Variables should be declared
// through this class. Then Importers, Generators, and Exporters will use those
// variables.
//
// Example usage:
//
//   Moriarty M;
//   M.SetName("Example Constraints")
//     .AddVariable("N", MInteger().Between(1, 100))
//     .AddVariable("A", MArray(MInteger().Between(3, 5)).OfLength("3 * N + 1"))
//     .AddVariable("S", MString().WithAlphabet("abc").OfLength("N"))
//     .AddGenerator("CornerCaseGenerator", CustomCornerCaseGenerator())
//     .AddGenerator("SmallExamples", SmallCaseGenerator());
class Moriarty {
 public:
  // SetName() [required]
  //
  // This is the name of this question. This is useful to distinguish different
  // questions (for interviews/competitions), CUJs, etc. The name is required
  // and is encoded into the random seed to ensure a different seed is provided
  // for each question.
  Moriarty& SetName(absl::string_view name);

  // SetNumCases() [optional]
  //
  // Sets an aspirational number of test cases to generate. All custom
  // generators will be called first, then specialized generators (min_, max_,
  // random_) will be called to increase the number of cases to `num_cases`. If
  // the custom generators produce more than `num_cases`, then those will all
  // still be generated. Setting `num_cases` = 0 means only your custom
  // generators will be run.
  //
  // Crashes on failure. See `TrySetNumCases()` for non-crashing version.
  Moriarty& SetNumCases(int num_cases);

  // TrySetNumCases() [optional]
  //
  // Sets an aspirational number of test cases to generate. All custom
  // generators will be called first, then specialized generators (min_, max_,
  // random_) will be called to increase the number of cases to `num_cases`. If
  // the custom generators produce more than `num_cases`, then those will all
  // still be generated. Setting `num_cases` = 0 means only your custom
  // generators will be run.
  //
  // Returns status on failure. See `SetNumCases()` for simpler API version.
  absl::Status TrySetNumCases(int num_cases);

  // SetSeed() [required]
  //
  // This is the seed used for random generation. The seed must:
  //  * Be at least 10 characters long.
  //
  // In the future, this may also be added as a requirement:
  //  * The first X characters must encode the `name` provided (this helps
  //    ensures a distinct seed is used for every question).
  //
  // Crashes on failure. See `TrySetSeed()` for non-crashing version.
  Moriarty& SetSeed(absl::string_view seed);

  // TrySetSeed()
  //
  // This is the seed used for random generation. The seed must:
  //  * Be at least 10 characters long.
  //
  // In the future, this may also be added as a requirement:
  //  * The first X characters must encode the `name` provided (this helps
  //    ensures a distinct seed is used for every question).
  //
  // Returns status on failure. See `SetSeed()` for simpler API version.
  absl::Status TrySetSeed(absl::string_view seed);

  // AddVariable()
  //
  // Adds a variable to Moriarty with all global constraints applied to it. For
  // example:
  //
  //  `M.AddVariable("N", MInteger().Between(1, 10));`
  //
  // means that *all* instances of N that are generated will be between 1
  // and 10. Additional local constraints can be added to this in specific
  // generators, but no generator may violate these constraints placed here.
  // (For example, if a generator says it wants N to be between 20 and 30, an
  // error will be thrown since there is no number that is between 1 and 10 AND
  // 20 and 30.)
  //
  // Variable names must start with a letter (A-Za-z), and then only contain
  // letters, numbers, and underscores (A-Za-z0-9_).
  //
  // TODO(darcybest): This currently does not check the first letter criteria
  // because one of our users has numbers first. All new users should only have
  // A-Za-z as first letters.
  //
  // Crashes on failure. See `TryGetVariable()` for non-crashing version.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  Moriarty& AddVariable(absl::string_view name, T variable);

  // TryAddVariable()
  //
  // Adds a variable to Moriarty with all global constraints applied to it. For
  // example:
  //
  //  `M.AddVariable("N", MInteger().Between(1, 10));`
  //
  // means that *all* instances of N that are generated will be between 1
  // and 10. Additional local constraints can be added to this in specific
  // generators, but no generator may violate these constraints placed here.
  // (For example, if a generator says it wants N to be between 20 and 30, an
  // error will be thrown since there is no number that is between 1 and 10 AND
  // 20 and 30.)
  //
  // Returns status on failure. See `AddVariable()` for simpler API version.
  template <typename T>
    requires std::derived_from<T,
                               librarian::MVariable<T, typename T::value_type>>
  absl::Status TryAddVariable(absl::string_view name, T variable);

  // AddGenerator()
  //
  // Adds a generator to Moriarty. `generator.Generate()` will be called to
  // create one or more `Case`s. This generator will be called `call_n_times`
  // times. Each call will use a different random seed.
  //
  // TODO(b/233664034): The following behavior may change in the future.
  // *Please do not depend on it.*
  //
  // If you copy this Moriarty class, the same Generator will be used in both
  // instances. For example,
  //   Moriarty M1 = Moriarty().AddGenerator("test", TestGenerator());
  //   Moriarty M2 = M1;
  //   M1.GenerateTestCases();  // Both of these calls will use the exact same
  //   M2.GenerateTestCases();  // TestGenerator under the hood.
  template <typename T>
    requires std::derived_from<T, Generator>
  Moriarty& AddGenerator(absl::string_view name, T generator,
                         int call_n_times = 1);

  // GenerateTestCases()
  //
  // Goes through all generators and generates all cases. All cases will be
  // stored internally and are ready to be exported or printed.
  //
  // Crashes on failure. See `TryGenerateTestCases()` for non-crashing version.
  void GenerateTestCases();

  // TryGenerateTestCases()
  //
  // Goes through all generators and generates all cases. All cases will be
  // stored internally and are ready to be exported or printed.
  //
  // Returns status on failure. See `GenerateTestCases()` for simpler API
  // version.
  absl::Status TryGenerateTestCases();

  // ExportTestCases()
  //
  // Exports all cases in order using the provided exporter. This exporter must
  // be derived from `Exporter`. `exporter.Export()` will be called exactly
  // once.
  template <typename T>
    requires std::derived_from<T, Exporter>
  void ExportTestCases(T exporter);

  // ImportTestCases()
  //
  // Imports all cases in order using the provided importer. This importer must
  // be derived from `Importer`. `importer.ImportTestCases()` will be called
  // exactly once.
  template <typename T>
    requires std::derived_from<T, Importer>
  absl::Status ImportTestCases(T importer);

  // TryValidateTestCases()
  //
  // Checks if all variables in all test cases are valid. If there are multiple
  // failures, this will return one of them.
  absl::Status TryValidateTestCases();

  // SetApproximateGenerationLimit()
  //
  // Sets a threshold for approximately how much data to generate. If set,
  // Moriarty will call generators until is has generated `limit` "units", where
  // a "unit" is computed as: 1 for integers, size for string, and sum of number
  // of units of its elements for arrays. All other MVariables will have a size
  // of 1.
  //
  // None of these values are guaranteed to remain the same in the future and
  // this function is a suggestion to Moriarty, not a guarantee that it will
  // stop generation at any point.
  void SetApproximateGenerationLimit(int64_t limit);

 private:
  // Seed info
  static constexpr int kMinimumSeedLength = 10;
  std::vector<int64_t> seed_;

  // Metadata
  std::string name_;
  int num_cases_ = 0;

  // Variables
  moriarty_internal::VariableSet variables_;

  // Generators
  struct GeneratorInfo {
    std::string name;
    // TODO(b/233664034): Consider changing to std::unique_ptr and changing
    // the copy constructor's behavior. pybind11 needs the copy constructor.
    std::shared_ptr<Generator> generator;
    int call_n_times;
  };
  std::vector<GeneratorInfo> generators_;
  std::optional<int64_t> approximate_generation_limit_;

  // TestCases
  std::vector<moriarty_internal::ValueSet> assigned_test_cases_;
  std::vector<TestCaseMetadata> test_case_metadata_;

  // Generates the seed for generator_[index]. Negative numbers are reserved
  // for specialized generators (e.g., min_, max_, random_ generators).
  absl::StatusOr<absl::Span<const int64_t>> GetSeedForGenerator(int index);

  // Determines if a single test case is valid
  absl::Status TryValidateSingleTestCase(
      const moriarty_internal::ValueSet& values);

  // Determines if a variable name is valid.
  static absl::Status ValidateVariableName(absl::string_view name);
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
Moriarty& Moriarty::AddVariable(absl::string_view name, T variable) {
  moriarty_internal::TryFunctionOrCrash(
      [&, this]() {
        return this->TryAddVariable<T>(name, std::move(variable));
      },
      "AddVariable");
  return *this;
}

template <typename T>
  requires std::derived_from<T, librarian::MVariable<T, typename T::value_type>>
absl::Status Moriarty::TryAddVariable(absl::string_view name, T variable) {
  MORIARTY_RETURN_IF_ERROR(ValidateVariableName(name));
  MORIARTY_RETURN_IF_ERROR(variables_.AddVariable(name, std::move(variable)))
      << "Adding the same variable multiple times";
  return absl::OkStatus();
}

template <typename T>
  requires std::derived_from<T, Generator>
Moriarty& Moriarty::AddGenerator(absl::string_view name, T generator,
                                 int call_n_times) {
  GeneratorInfo gen;
  gen.name = name;
  gen.generator = std::make_unique<T>(std::move(generator));
  gen.call_n_times = call_n_times;
  generators_.push_back(std::move(gen));

  return *this;
}

template <typename T>
  requires std::derived_from<T, Exporter>
void Moriarty::ExportTestCases(T exporter) {
  moriarty_internal::ExporterManager manager(&exporter);
  manager.SetAllValues(assigned_test_cases_);
  manager.SetTestCaseMetadata(test_case_metadata_);
  manager.SetGeneralConstraints(variables_);

  exporter.ExportTestCases();
}

template <typename T>
  requires std::derived_from<T, Importer>
absl::Status Moriarty::ImportTestCases(T importer) {
  moriarty_internal::ImporterManager(&importer).SetGeneralConstraints(
      variables_);

  MORIARTY_RETURN_IF_ERROR(importer.ImportTestCases()) << "Importer failed.";
  for (moriarty_internal::ValueSet values :
       moriarty_internal::ImporterManager(&importer).GetTestCases()) {
    assigned_test_cases_.push_back(std::move(values));
    test_case_metadata_.push_back(
        TestCaseMetadata().SetTestCaseNumber(assigned_test_cases_.size()));
  }

  return absl::OkStatus();
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_MORIARTY_H_
