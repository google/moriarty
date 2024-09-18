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

#ifndef MORIARTY_SRC_VARIABLES_CONSTRAINTS_CONTAINERS_H_
#define MORIARTY_SRC_VARIABLES_CONSTRAINTS_CONTAINERS_H_

#include <concepts>

#include "absl/strings/string_view.h"
#include "src/variables/constraints/base_constraints.h"
#include "src/variables/minteger.h"

namespace moriarty {

// Constraint stating that the container must have this length.
class Length : public MConstraint {
 public:
  // The length must be exactly this value.
  // E.g., Length(10)
  template <typename Integer>
    requires std::integral<Integer>
  explicit Length(Integer value);

  // The length must be exactly this integer expression.
  // E.g., Length("3 * N + 1").
  explicit Length(absl::string_view expression);

  // The length must satisfy all of these constraints.
  // E.g., Length(Between(1, 10), Prime())
  template <typename... Constraints>
    requires(std::constructible_from<MInteger, Constraints...> &&
             sizeof...(Constraints) > 0)
  explicit Length(Constraints&&... constraints);

  // Returns the constraints on the length.
  [[nodiscard]] MInteger GetConstraints() const;

 private:
  MInteger length_;
};

// Constraints that elements of a container must satisfy.
template <typename MElementType>
class Elements : public MConstraint {
 public:
  // The elements of the container must satisfy all of these constraints.
  // E.g., Elements<MInteger>(Between(1, 10), Prime())
  template <typename... MElementTypeConstraints>
    requires(
        std::constructible_from<MElementType, MElementTypeConstraints...> &&
        sizeof...(MElementTypeConstraints) > 0)
  explicit Elements(MElementTypeConstraints&&... element_constraints);

  // Returns the constraints on the elements.
  [[nodiscard]] MElementType GetConstraints() const;

 private:
  MElementType element_constraints_;
};

// Constraint stating that the elements of a container must be distinct.
class DistinctElements : public MConstraint {
 public:
  // The elements of the container must all be distinct.
  explicit DistinctElements() = default;
};

// -----------------------------------------------------------------------------
//  Template Implementation Below

template <typename Integer>
  requires std::integral<Integer>
Length::Length(Integer value) : length_(Exactly(value)) {}

template <typename... Constraints>
  requires(std::constructible_from<MInteger, Constraints...> &&
           sizeof...(Constraints) > 0)
Length::Length(Constraints&&... constraints)
    : length_(std::forward<Constraints>(constraints)...) {}

template <typename MElementType>
template <typename... MElementTypeConstraints>
  requires(std::constructible_from<MElementType, MElementTypeConstraints...> &&
           sizeof...(MElementTypeConstraints) > 0)
Elements<MElementType>::Elements(
    MElementTypeConstraints&&... element_constraints)
    : element_constraints_(
          std::forward<MElementTypeConstraints>(element_constraints)...) {}

template <typename MElementType>
MElementType Elements<MElementType>::GetConstraints() const {
  return element_constraints_;
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_CONSTRAINTS_CONTAINERS_H_
