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

#include "src/variables/constraints/size_constraints.h"

#include <string>

#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/librarian/size_property.h"

namespace moriarty {

SizeCategory::SizeCategory(CommonSize size) : size_(size) {}

SizeCategory::SizeCategory(absl::string_view size)
    : size_(librarian::CommonSizeFromString(size)) {}

SizeCategory SizeCategory::Any() { return SizeCategory(CommonSize::kAny); }
SizeCategory SizeCategory::Min() { return SizeCategory(CommonSize::kMin); }
SizeCategory SizeCategory::Tiny() { return SizeCategory(CommonSize::kTiny); }
SizeCategory SizeCategory::Small() { return SizeCategory(CommonSize::kSmall); }
SizeCategory SizeCategory::Medium() {
  return SizeCategory(CommonSize::kMedium);
}
SizeCategory SizeCategory::Large() { return SizeCategory(CommonSize::kLarge); }
SizeCategory SizeCategory::Huge() { return SizeCategory(CommonSize::kHuge); }
SizeCategory SizeCategory::Max() { return SizeCategory(CommonSize::kMax); }

CommonSize SizeCategory::GetCommonSize() const { return size_; }

std::string SizeCategory::ToString() const {
  return absl::Substitute("SizeCategory($0)", librarian::ToString(size_));
}

}  // namespace moriarty
