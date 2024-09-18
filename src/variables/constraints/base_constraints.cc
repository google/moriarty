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

#include "src/variables/constraints/base_constraints.h"

#include <string>

#include "absl/strings/string_view.h"

namespace moriarty {

Exactly<std::string>::Exactly(absl::string_view value)
    : value_(std::string(value)) {}

std::string Exactly<std::string>::GetValue() const { return value_; }

}  // namespace moriarty
