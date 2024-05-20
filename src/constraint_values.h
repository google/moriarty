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

#ifndef MORIARTY_SRC_CONSTRAINT_VALUES_H_
#define MORIARTY_SRC_CONSTRAINT_VALUES_H_

#include <string_view>

#include "absl/log/absl_check.h"
#include "absl/status/statusor.h"
#include "src/internal/universe.h"

namespace moriarty {

// ConstraintValues
//
// This class should not be used instantiated by the user. It should only be
// used as an argument in the custom constraint function of a variable, to
// obtain the value of other variables in the universe.
//
// E.g.,
//
// Moriarty M;
// M.AddVariable("N", MInteger().Between(1, 50));
// M.AddVariable("X", MInteger().Between(1, 100).AddCustomConstraint(
//           "NotMultipleOfN", {"N"}, [](int x, const ConstraintValues& values)
//           {
//             return x % values.GetValue<MInteger>("N") != 0;
//           }));
class ConstraintValues {
 public:
  // This class does not take ownership of `universe`.
  explicit ConstraintValues(moriarty_internal::Universe* universe)
      : universe_(*universe) {}

  // GetValue<>()
  //
  // Returns the value assigned to a variable that is in the universe described
  // by `variable_name`. If `variable_name` does not exist in the universe,
  // or the value of the variable fails to cast to the typename specified, this
  // function crashes.
  template <typename MType>
  typename MType::value_type GetValue(std::string_view variable_name) const;

 private:
  moriarty_internal::Universe& universe_;
};

// -----------------------------------------------------------------------------
//  Template implementation below

template <typename MType>
typename MType::value_type ConstraintValues::GetValue(
    std::string_view variable_name) const {
  // Need to ensure we are getting the correct the value type.
  absl::StatusOr<typename MType::value_type> var =
      universe_.GetValue<MType>(variable_name);
  // We need to force a crash if it fails, since we don't want the user to
  // handle the StatusOr<> in their defined custom constraint function.
  ABSL_CHECK_OK(var);
  return *var;
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_CONSTRAINT_VALUES_H_
