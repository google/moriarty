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

#ifndef MORIARTY_SRC_PROPERTY_H_
#define MORIARTY_SRC_PROPERTY_H_

#include <string>

namespace moriarty {

// Property
//
// A specific property for a variable to have.
//  Example: {.category = "size", .descriptor = "small"}
struct Property {
  std::string category;
  std::string descriptor;

  enum Enforcement { kFailIfUnknown, kIgnoreIfUnknown };
  Enforcement enforcement = kFailIfUnknown;

  std::string ToString() const;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_PROPERTY_H_
