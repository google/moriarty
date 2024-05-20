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

#ifndef MORIARTY_SRC_LIBRARIAN_SUBVALUES_H_
#define MORIARTY_SRC_LIBRARIAN_SUBVALUES_H_

#include <memory>
#include <string>
#include <utility>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "src/internal/abstract_variable.h"

namespace moriarty {

namespace moriarty_internal {
class SubvaluesManager;  // Forward declaring the internal API
}  // namespace moriarty_internal

namespace librarian {

// Subvalues [Class]
//
// A storage for subvalues of an MVariable. For example, the value of a
// string's length, the value of an array's elements, the value of a proto's
// field called `num_happiness`, etc.
//
// See `TryAddSubvalue()` for how to use.
class Subvalues {
 public:
  // AddSubvalue()
  //
  // Adds `subvalue` as a subvalue. Subvalues should be an actual value
  // (int64_t, std::string, etc). not a Moriarty type (MInteger, MString, etc).
  //
  // TODO(darcybest): Determine semantics around duplicated names.
  template <typename MType>
  Subvalues& AddSubvalue(absl::string_view subvalue_name,
                         MType::value_type subvalue);

 private:
  absl::flat_hash_map<std::string, moriarty_internal::VariableValue> subvalues_;

  // ---------------------------------------------------------------------------
  //    Start of Internal Extended API
  //
  // These functions can be accessed via
  // `moriarty_internal::SubvariableManager`. Users and Librarians should not
  // need to access these functions. See `SubvariableManager` for more details.
  friend class moriarty_internal::SubvaluesManager;

  // GetSubvalue()
  //
  // Returns the subvalue with name `subvalue_name` or `std::nullopt` if that
  // subvalue is unknown. Ownership of the underlying AbstractVariable is not
  // transferred to the caller.
  absl::StatusOr<absl::Nonnull<const moriarty_internal::VariableValue*>>
  GetSubvalue(absl::string_view subvalue_name) const;
};

}  // namespace librarian

namespace moriarty_internal {

class SubvaluesManager {
 public:
  // This class does not take ownership of `subvariable_to_manage`
  explicit SubvaluesManager(librarian::Subvalues* subvalues_to_manage);

  absl::StatusOr<absl::Nonnull<const moriarty_internal::VariableValue*>>
  GetSubvalue(absl::string_view subvalue_name) const;

 private:
  librarian::Subvalues& managed_subvalues_;
};

}  // namespace moriarty_internal

namespace librarian {

template <typename MType>
Subvalues& Subvalues::AddSubvalue(absl::string_view subvalue_name,
                                  MType::value_type subvalue) {
  subvalues_.insert_or_assign(subvalue_name,
                              {.variable = std::make_shared<const MType>(),
                               .value = std::move(subvalue)});
  return *this;
}

}  // namespace librarian
}  // namespace moriarty

#endif  // MORIARTY_SRC_LIBRARIAN_SUBVALUES_H_
