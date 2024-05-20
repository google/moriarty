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

#include "src/scenario.h"

#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "src/property.h"

namespace moriarty {

Scenario& Scenario::WithGeneralProperty(Property property) {
  properties_.push_back(std::move(property));
  return *this;
}

Scenario& Scenario::WithTypeSpecificProperty(absl::string_view mvariable_type,
                                             Property property) {
  type_specific_properties_[mvariable_type].push_back(std::move(property));
  return *this;
}

std::vector<Property> Scenario::GetGeneralProperties() const {
  return properties_;
}

std::vector<Property> Scenario::GetTypeSpecificProperties(
    absl::string_view mvariable_type) const {
  auto it = type_specific_properties_.find(mvariable_type);
  if (it == type_specific_properties_.end()) return {};
  return it->second;
}

}  // namespace moriarty
