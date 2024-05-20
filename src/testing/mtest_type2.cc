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

#include "src/testing/mtest_type2.h"

#include <stdint.h>

#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/subvalues.h"
#include "src/property.h"
#include "src/util/status_macro/status_macros.h"
#include "src/variables/minteger.h"

// LINT.IfChange

namespace moriarty_testing {

using ::moriarty::MInteger;
using ::moriarty::librarian::IOConfig;
using ::moriarty::librarian::Subvalues;

MTestType2::MTestType2() {
  RegisterKnownProperty("size", &MTestType2::WithSizeProperty);
}

absl::StatusOr<TestType2> MTestType2::ReadImpl() {
  MORIARTY_ASSIGN_OR_RETURN(moriarty::librarian::IOConfig * io_config,
                            GetIOConfig());
  MORIARTY_ASSIGN_OR_RETURN(std::string token, io_config->ReadToken());
  std::stringstream is(token);

  int64_t value;
  if (!(is >> value))
    return absl::InvalidArgumentError("Unable to read a TestType2.");
  return value;
}

absl::Status MTestType2::PrintImpl(const TestType2& value) {
  MORIARTY_ASSIGN_OR_RETURN(moriarty::librarian::IOConfig * io_config,
                            GetIOConfig());
  return io_config->PrintToken(std::to_string(value));
}

absl::Status MTestType2::MergeFromImpl(const MTestType2& other) {
  merged_ = true;
  return absl::OkStatus();
}

bool MTestType2::WasMerged() const { return merged_; }

// Acceptable `KnownProperty`s: "size":"small"/"large"
absl::Status MTestType2::WithSizeProperty(moriarty::Property property) {
  // Size is the only known property
  ABSL_CHECK_EQ(property.category, "size");

  if (property.descriptor == "small")
    Is(kGeneratedValueSmallSize);
  else if (property.descriptor == "large")
    Is(kGeneratedValueLargeSize);
  else
    return absl::InvalidArgumentError(absl::Substitute(
        "Unknown property descriptor: $0", property.descriptor));

  return absl::OkStatus();
}

MTestType2& MTestType2::SetAdder(absl::string_view variable_name) {
  adder_variable_name_ = variable_name;
  return *this;
}

// My value is multiplied by this value
MTestType2& MTestType2::SetMultiplier(moriarty::MInteger multiplier) {
  multiplier_ = multiplier;
  return *this;
}

// I am == "multiplier * value + other_variable", so to be valid,
//   (valid - other_variable) / multiplier
// must be an integer.
absl::Status MTestType2::IsSatisfiedWithImpl(const TestType2& value) const {
  TestType2 val = value;
  if (adder_variable_name_) {
    MORIARTY_ASSIGN_OR_RETURN(
        TestType2 subtract_me, GetKnownValue<MTestType2>(*adder_variable_name_),
        _ << "Unknown adder variable: " << *adder_variable_name_);
    val = val - subtract_me;
  }

  // 0 is a multiple of all numbers!
  if (val == 0) return absl::OkStatus();

  {
    // value must be a multiple of `multiplier`. So I'll go through all my
    // factors and one of those must be in the range...
    if (val < 0) val = -val;
    bool found_divisor = false;
    for (int div = 1; div * div <= val; div++) {
      if (val % div == 0) {
        absl::Status status1 = SatisfiesConstraints(multiplier_, div);
        if (!status1.ok() && !moriarty::IsUnsatisfiedConstraintError(status1))
          return status1;

        absl::Status status2 = SatisfiesConstraints(multiplier_, val / div);
        if (!status2.ok() && !moriarty::IsUnsatisfiedConstraintError(status2))
          return status2;

        if (status1.ok() || status2.ok()) found_divisor = true;
      }
    }
    if (!found_divisor)
      return moriarty::UnsatisfiedConstraintError(
          absl::Substitute("$0 is not a multiple of any valid multiplier.",
                           static_cast<int>(val)));
  }

  return absl::OkStatus();
}

moriarty::librarian::IOConfig::WhitespacePolicy
MTestType2::GetWhitespacePolicy() {
  IOConfig* io_config = *GetIOConfig();  // Hides a StatusOr
  return io_config->GetWhitespacePolicy();
}

std::vector<std::string> MTestType2::GetDependenciesImpl() const {
  return GetDependencies(multiplier_);
}

absl::StatusOr<Subvalues> MTestType2::GetSubvaluesImpl(
    const TestType2& value) const {
  TestType2 addition = 0;
  if (adder_variable_name_) {
    MORIARTY_ASSIGN_OR_RETURN(addition,
                              GetKnownValue<MTestType2>(*adder_variable_name_));
  }
  return Subvalues().AddSubvalue<MInteger>(
      "multiplier", (value - addition.value) / kGeneratedValue);
}

// Always returns pi. Does not directly depend on `rng`, but we generate a
// random number between 1 and 1 (aka, 1) to ensure the RandomEngine is
// available for use if we wanted to.
absl::StatusOr<TestType2> MTestType2::GenerateImpl() {
  TestType2 addition = 0;
  if (adder_variable_name_) {
    MORIARTY_ASSIGN_OR_RETURN(addition,
                              GenerateValue<MTestType2>(*adder_variable_name_));
  }

  MORIARTY_ASSIGN_OR_RETURN(int64_t multiplier,
                            Random("multiplier", multiplier_));
  return kGeneratedValue * multiplier + addition;
}

absl::StatusOr<std::vector<MTestType2>> MTestType2::GetDifficultInstancesImpl()
    const {
  return std::vector<MTestType2>({MTestType2().Is(2), MTestType2().Is(3)});
}

}  // namespace moriarty_testing

// LINT.ThenChange(mtest_type.cc)
