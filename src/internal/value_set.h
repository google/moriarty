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

#ifndef MORIARTY_SRC_INTERNAL_VALUE_SET_H_
#define MORIARTY_SRC_INTERNAL_VALUE_SET_H_

#include <any>
#include <concepts>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"

namespace moriarty {
namespace moriarty_internal {

// ValueSet
//
// A cookie jar that simply stores type-erased values. You can only put things
// in and take them out. Nothing more. Logically equivalent to
//
//   std::map<std::string, std::any>
class ValueSet {
 public:
  // Set()
  //
  // Sets variable_name to `value`. If this was set previously, it will be
  // overwritten.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  void Set(absl::string_view variable_name, T::value_type value);

  // Get()
  //
  // Returns the stored value for the variable `variable_name`.
  //
  //  * If `variable_name` is non-existent, returns `ValueNotFoundError()`.
  //  * If the value cannot be converted to T, returns kFailedPrecondition.
  template <typename T>
    requires std::derived_from<T, AbstractVariable>
  absl::StatusOr<typename T::value_type> Get(
      absl::string_view variable_name) const;

  // UnsafeGet()
  //
  // Returns the std::any value for the variable `variable_name`. Should only be
  // used when the value type is unknown.
  //
  //  Returns ValueNotFoundError if the variable has not been set.
  absl::StatusOr<std::any> UnsafeGet(absl::string_view variable_name) const;

  // Contains()
  //
  // Determines if `variable_name` is in this ValueSet.
  bool Contains(absl::string_view variable_name) const;

  // Erase()
  //
  // Deletes the stored value for the variable `variable_name`. If
  // `variable_name` is non-existent, this is a no-op.
  void Erase(absl::string_view variable_name);

  // GetApproximateSize()
  //
  // The "size" of a value set is a rough approximation of how large the whole
  // object is. In general, it is the number of integers, characters, etc. This
  // only behaves as expected for primitive types. All other types have a "size"
  // of 1. This may change in the future. Calling `Set()` on a variable multiple
  // times will affect the size multiple times, which is probably not intended.
  //
  // TODO(darcybest): Currently, this only handles array and string separately.
  // In the future, we may want to consider each MVariable type to have its own
  // `ApproximateSize()`. I don't want to put this directly into MVariable yet
  // since we are not sure if this is a long term solution.
  int64_t GetApproximateSize() const { return approximate_size_; }

 private:
  absl::flat_hash_map<std::string, std::any> values_;

  int64_t approximate_size_ = 0;

  template <typename T>
  int64_t ApproximateSize(const T& value) const;

  template <typename T>
  int64_t ApproximateSize(const std::vector<T>& values) const;

  int64_t ApproximateSize(const std::string& value) const;
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename T>
  requires std::derived_from<T, AbstractVariable>
absl::StatusOr<typename T::value_type> ValueSet::Get(
    absl::string_view variable_name) const {
  auto it = values_.find(variable_name);
  if (it == values_.end()) return ValueNotFoundError(variable_name);

  using TV = typename T::value_type;
  const TV* val = std::any_cast<const TV>(&it->second);
  if (val == nullptr)
    return absl::FailedPreconditionError(
        absl::Substitute("Unable to cast $0", variable_name));

  return *val;
}

template <typename T>
  requires std::derived_from<T, AbstractVariable>
void ValueSet::Set(absl::string_view variable_name, T::value_type value) {
  approximate_size_ += ApproximateSize(value);
  values_[variable_name] = std::move(value);
}

template <typename T>
int64_t ValueSet::ApproximateSize(const T& value) const {
  return 1;
}

template <typename T>
int64_t ValueSet::ApproximateSize(const std::vector<T>& values) const {
  int64_t size = 0;
  for (const T& x : values) size += ApproximateSize(x);
  return size;
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_VALUE_SET_H_
