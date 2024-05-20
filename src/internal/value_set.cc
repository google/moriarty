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

#include "src/internal/value_set.h"

#include <any>
#include <cstdint>
#include <string>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/errors.h"

namespace moriarty {
namespace moriarty_internal {

bool ValueSet::Contains(absl::string_view variable_name) const {
  return values_.contains(variable_name);
}

void ValueSet::Erase(absl::string_view variable_name) {
  auto it = values_.find(variable_name);
  if (it == values_.end()) return;

  // TODO(darcybest): If we need a better approximation, we should store the
  // size of the objects as they are `Set` and subtract the proper size rather
  // than just 1.
  approximate_size_--;
  values_.erase(it);
}

absl::StatusOr<std::any> ValueSet::UnsafeGet(
    absl::string_view variable_name) const {
  auto it = values_.find(variable_name);
  if (it == values_.end()) return ValueNotFoundError(variable_name);
  return it->second;
}

int64_t ValueSet::ApproximateSize(const std::string& value) const {
  return value.size();
}

}  // namespace moriarty_internal
}  // namespace moriarty
