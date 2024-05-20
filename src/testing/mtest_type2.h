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

#ifndef MORIARTY_SRC_TESTING_MTEST_TYPE2_H_
#define MORIARTY_SRC_TESTING_MTEST_TYPE2_H_

#include <stdint.h>

#include <optional>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/librarian/subvalues.h"
#include "src/property.h"
#include "src/variables/minteger.h"

// This is identical to mtest_type.h, just with different constants (e.g.,
// `kGeneratedValue`) for generation.

// LINT.IfChange

namespace moriarty_testing {

// TestType2 [for internal tests only]
//
// Simple data type that behaves (almost) exactly like an `int`.
// The variable `MTestType2` will generate this.
//   `int64_t` is to `MInteger` as `TestType2` is to `MTestType2`
struct TestType2 {
  // NOLINTNEXTLINE: FakeDataType *is* an integer (and is only used in testing)
  TestType2(int val = 0) : value(val) {}

  // NOLINTNEXTLINE: FakeDataType *is* an integer (and is only used in testing)
  operator int() const { return value; }

  int value;
};

// MTestType2 [for internal tests only]
//
// A bare bones Moriarty variable.
//
// Basics of this type:
//   - Returns a single (pi-like) value when Generate is called.
//   - Has the property "size" with "small" and "large".
//   - Read/Print behave like integers.
//   - You can depend on another MTestType2, and be pi + that_value.
//       (Example, if X's `adder` is Y, and the value of Y is 123, then X's
//       value is the default plus 123)
//   - Has a subvariable called `multiplier`. If X's `multiplier` is 3, then
//     this value is 3 * (default_value + adder_value).
//
// Summary: The generated value is:
//   multiplier * (default_value + adder_value)
class MTestType2
    : public moriarty::librarian::MVariable<MTestType2, TestType2> {
 public:
  // This value is the default value returned from Generate.
  static constexpr int64_t kGeneratedValue = 2718281;
  // This value is returned when Property size = "small"
  static constexpr int64_t kGeneratedValueSmallSize = 111;
  // This value is returned when Property size = "large"
  static constexpr int64_t kGeneratedValueLargeSize = 999999;
  // These two values are returned as corner cases.
  static constexpr int64_t kCorner1 = 99991;
  static constexpr int64_t kCorner2 = 99992;

  MTestType2();

  std::string Typename() const override { return "MTestType2"; };

  absl::StatusOr<TestType2> ReadImpl() override;

  absl::Status PrintImpl(const TestType2& value) override;

  absl::Status MergeFromImpl(const MTestType2& other) override;

  bool WasMerged() const;

  // Acceptable `KnownProperty`s: "size":"small"/"large"
  absl::Status WithSizeProperty(moriarty::Property property);

  // My value is increased by this other variable's value
  MTestType2& SetAdder(absl::string_view variable_name);

  // My value is multiplied by this value
  MTestType2& SetMultiplier(moriarty::MInteger multiplier);

  // I am == "multiplier * value + other_variable", so to be valid,
  //   (valid - other_variable) / multiplier
  // must be an integer.
  absl::Status IsSatisfiedWithImpl(const TestType2& value) const override;

  moriarty::librarian::IOConfig::WhitespacePolicy GetWhitespacePolicy();

  absl::StatusOr<std::vector<MTestType2>> GetDifficultInstancesImpl()
      const override;

 private:
  moriarty::MInteger multiplier_ = moriarty::MInteger().Between(1, 1);
  std::optional<std::string> adder_variable_name_;

  bool merged_ = false;

  absl::StatusOr<moriarty::librarian::Subvalues> GetSubvaluesImpl(
      const TestType2& value) const override;

  // Always returns pi. Does not directly depend on `rng`, but we generate a
  // random number between 1 and 1 (aka, 1) to ensure the RandomEngine is
  // available for use if we wanted to.
  absl::StatusOr<TestType2> GenerateImpl() override;

  std::vector<std::string> GetDependenciesImpl() const override;
};

}  // namespace moriarty_testing

// LINT.ThenChange(mtest_type.h)

#endif  // MORIARTY_SRC_TESTING_MTEST_TYPE2_H_
