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

#include "src/internal/variable_name_utils.h"

#include <optional>
#include <string>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace moriarty {
namespace moriarty_internal {

absl::string_view BaseVariableName(absl::string_view variable_name) {
  return variable_name.substr(0, variable_name.find('.'));
}

std::string ConstructVariableName(absl::string_view base_variable_name,
                                  absl::string_view subvariable_name) {
  return absl::StrCat(base_variable_name, ".", subvariable_name);
}

std::string ConstructVariableName(
    const VariableNameBreakdown& variable_name_breakdown) {
  if (variable_name_breakdown.subvariable_name.has_value()) {
    return absl::StrCat(variable_name_breakdown.base_variable_name, ".",
                        *variable_name_breakdown.subvariable_name);
  }
  return variable_name_breakdown.base_variable_name;
}

VariableNameBreakdown CreateVariableNameBreakdown(
    absl::string_view variable_name) {
  VariableNameBreakdown ret;
  ret.base_variable_name = BaseVariableName(variable_name);
  std::optional<absl::string_view> subvariable_name =
      SubvariableName(variable_name);
  if (subvariable_name.has_value()) {
    ret.subvariable_name = *subvariable_name;
  }
  return ret;
}

bool HasSubvariable(absl::string_view variable_name) {
  return absl::StrContains(variable_name, '.');
}

std::optional<absl::string_view> SubvariableName(
    absl::string_view variable_name) {
  auto pos = variable_name.find('.');
  if (pos == absl::string_view::npos) {
    return std::nullopt;
  }

  return variable_name.substr(pos + 1);
}

}  // namespace moriarty_internal
}  // namespace moriarty
