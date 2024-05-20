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

#ifndef MORIARTY_SRC_INTERNAL_VARIABLE_NAME_UTILS_H_
#define MORIARTY_SRC_INTERNAL_VARIABLE_NAME_UTILS_H_

#include <optional>
#include <string>

#include "absl/strings/string_view.h"

namespace moriarty {
namespace moriarty_internal {

struct VariableNameBreakdown {
  std::string base_variable_name;
  std::optional<std::string> subvariable_name;

  friend bool operator==(const VariableNameBreakdown& a,
                         const VariableNameBreakdown& b) {
    return a.base_variable_name == b.base_variable_name &&
           a.subvariable_name == b.subvariable_name;
  }
};

// BaseVariableName()
//
// Given a variable name, returns the substring that corresponds to the
// base variable where '.' separates subvariable names. For example if the
// variable name is "A.0.length" then the base variable name is "A".
absl::string_view BaseVariableName(absl::string_view variable_name);

// ConstructVariableName()
//
// Constructs a variable name given the base variable name and the subvariable
// name with a `.`. For example, if you have a variable named "A" and a
// subvariable "B" then this will return "A.B".
std::string ConstructVariableName(absl::string_view base_variable_name,
                                  absl::string_view subvariable_name);

// ConstructVariableName()
//
// Constructs a variable name given the variable name breakdown, concatenating
// with a `.`. For example, if you have base_variable_name "A" and a
// subvariable_name "B" then this will return "A.B".
std::string ConstructVariableName(const VariableNameBreakdown& variable_name);

// CreateVariableNameBreakdown()
//
// Creates a VariableNameBreakdown struct based on the variable name. This
// should be used if you expect to use both the BaseVariableName() and
// SubvariableName() instead of calling each separately.
VariableNameBreakdown CreateVariableNameBreakdown(
    absl::string_view variable_name);

// HasSubvariable()
//
// Returns true if the variable name has a subvariable component. This is
// indicated by having a `.` in the name. For example the name "A.B" returns
// true while "ABC" returns false.
bool HasSubvariable(absl::string_view variable_name);

// SubvariableName()
//
// Given a variable name, returns the substring (if one exists) that corresponds
// to the subvariable (relative to the base variable) where '.' separates
// subvariable names. For example if the variable name is "A.0.length" then the
// subvariable name is "0.length". If the variable name is "A" then there is no
// subvariable name.
std::optional<absl::string_view> SubvariableName(
    absl::string_view variable_name);

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_VARIABLE_NAME_UTILS_H_
