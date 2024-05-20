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

#include "src/librarian/subvalues.h"

#include "absl/base/nullability.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/internal/abstract_variable.h"

namespace moriarty {
namespace librarian {

absl::StatusOr<absl::Nonnull<const moriarty_internal::VariableValue*>>
Subvalues::GetSubvalue(absl::string_view subvalue_name) const {
  auto it = subvalues_.find(subvalue_name);
  if (it == subvalues_.end())
    return absl::NotFoundError(
        absl::Substitute("Subvalue '$0' not found", subvalue_name));
  return &it->second;
}

}  // namespace librarian

namespace moriarty_internal {

SubvaluesManager::SubvaluesManager(librarian::Subvalues* subvalues_to_manage)
    : managed_subvalues_(*subvalues_to_manage) {}

absl::StatusOr<absl::Nonnull<const moriarty_internal::VariableValue*>>
SubvaluesManager::GetSubvalue(absl::string_view subvalue_name) const {
  return managed_subvalues_.GetSubvalue(subvalue_name);
}

}  // namespace moriarty_internal

}  // namespace moriarty
