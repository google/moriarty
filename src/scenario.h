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

#ifndef MORIARTY_SRC_SCENARIO_H_
#define MORIARTY_SRC_SCENARIO_H_

#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "src/property.h"

namespace moriarty {

// Scenario
//
// Contains a description of all properties that you would like to test in a
// specific situation. Each property can be for all variables or for just some
// specific types (e.g., just for integers and strings).
class Scenario {
 public:
  // WithGeneralProperty()
  //
  // Specifies that `property` is a part of this scenario and should be applied
  // to all mvariable types.
  Scenario& WithGeneralProperty(Property property);

  // WithTypeSpecificProperty()
  //
  // Specifies that all variables of type `mvariable_type` have `property`.
  // `mvariable_type` will be checked against a variable's `Typename()`. Some
  // examples are `MInteger`, `MString`, `MArray<MString>`.
  Scenario& WithTypeSpecificProperty(absl::string_view mvariable_type,
                                     Property property);

  // GetGeneralProperties()
  //
  // Returns a list of all properties that apply to all types.
  std::vector<Property> GetGeneralProperties() const;

  // GetTypeSpecificProperties()
  //
  // Returns a list of all properties that are for `mvariable_type` (not
  // including the general ones).
  std::vector<Property> GetTypeSpecificProperties(
      absl::string_view mvariable_type) const;

 private:
  std::vector<Property> properties_;
  absl::flat_hash_map<std::string, std::vector<Property>>
      type_specific_properties_;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_SCENARIO_H_
