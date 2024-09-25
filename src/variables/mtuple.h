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

#ifndef MORIARTY_SRC_VARIABLES_MTUPLE_H_
#define MORIARTY_SRC_VARIABLES_MTUPLE_H_

#include <concepts>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/substitute.h"
#include "src/errors.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/property.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty {

// MTuple<>
//
// Describes constraints placed on an ordered tuple of objects. All objects in
// the tuple must have a corresponding MVariable.
//
// This can hold as many objects as you'd like. For example:
//
//    MTuple<MInteger, MInteger>
// or
//    MTuple<
//          MArray<MInteger>,
//          MArray<MTuple<MInteger, MInteger, MString>>,
//          MTuple<MInteger, MString>
//         >
template <typename... MElementTypes>
class MTuple : public librarian::MVariable<
                   MTuple<MElementTypes...>,
                   std::tuple<typename MElementTypes::value_type...>> {
 public:
  using tuple_value_type = std::tuple<typename MElementTypes::value_type...>;

  MTuple();
  explicit MTuple(MElementTypes... values);

  // Typename()
  //
  // Returns a string representing the name of this type (for example,
  // "MTuple<MInteger, MInteger, MString>"). This is mostly used for
  // debugging/error messages.
  [[nodiscard]] std::string Typename() const override;

  // Of()
  //
  // Sets the argument matching `index` in the tuple.
  //
  // Example:
  //   Of<0>(MInteger().Between(1, 10));
  //
  // TODO(b/208296530): We should be taking the name as a parameter instead of
  // the index as a template argument.
  // TODO(b/208296530): `Of()` is a pretty bad name... We should change it to
  // something more meaningful.
  template <int index, typename T>
  MTuple& Of(T variable);

  // WithSeparator()
  //
  // Sets the whitespace separator to be used between different elements
  // when reading/writing. Default = kSpace.
  MTuple& WithSeparator(Whitespace separator);

  // OfSizeProperty()
  //
  // Tells each component that they should each have this size property.
  // `property.category` must be "size". The exact values here are not
  // guaranteed and may change over time. If exact values are required, specify
  // them manually.
  absl::Status OfSizeProperty(Property property);

 private:
  std::tuple<MElementTypes...> elements_;

  std::optional<Whitespace> separator_;
  Whitespace GetSeparator() const;

  // Pass `property` along to all variables in this tuple.
  absl::Status DistributePropertyToValues(Property property);
  template <std::size_t... I>
  absl::Status DistributePropertyToValues(Property property,
                                          std::index_sequence<I...>);

  template <std::size_t I>
  absl::Status GenerateSingleElement(tuple_value_type& result);
  template <std::size_t I>
  absl::Status TryReadAndSet(tuple_value_type& read_values);
  template <std::size_t I>
  absl::Status PrintElementWithLeadingSeparator(const tuple_value_type& value);

  // ---------------------------------------------------------------------------
  //  MVariable overrides
  absl::StatusOr<tuple_value_type> GenerateImpl() override;
  absl::Status IsSatisfiedWithImpl(
      const tuple_value_type& value) const override;
  absl::Status MergeFromImpl(const MTuple& other) override;
  absl::StatusOr<tuple_value_type> ReadImpl() override;
  absl::Status PrintImpl(const tuple_value_type& value) override;
  std::vector<std::string> GetDependenciesImpl() const override;
  // ---------------------------------------------------------------------------

  // ---------------------------------------------------------------------------
  //  MVariable overrides, indices versions
  //  * For tuple, a version of each function is needed that has an
  //    integer_sequence. This is just simply a list of {0, 1, ...} of the
  //    appropriate size so parameter packs can be used, and be run at compile
  //    time.
  template <std::size_t... I>
  absl::StatusOr<tuple_value_type> GenerateImpl(std::index_sequence<I...>);
  template <std::size_t... I>
  absl::Status IsSatisfiedWithImpl(const tuple_value_type& value,
                                   std::index_sequence<I...>) const;
  template <std::size_t... I>
  absl::Status MergeFromImpl(const MTuple& other, std::index_sequence<I...>);
  template <std::size_t... I>
  absl::StatusOr<tuple_value_type> ReadImpl(std::index_sequence<I...>);
  template <std::size_t... I>
  absl::Status PrintImpl(const tuple_value_type& value,
                         std::index_sequence<I...>);
  template <std::size_t... I>
  std::vector<std::string> GetDependenciesImpl(std::index_sequence<I...>) const;
  // ---------------------------------------------------------------------------
};

// Class template argument deduction (CTAD). Allows for `Array(MInteger())`
// instead of `Array<MInteger>(MInteger())`. See `NestedArray()` for nesting
// multiple `Arrays` inside one another.
template <typename... MElementTypes>
MTuple(MElementTypes...) -> MTuple<MElementTypes...>;

// -----------------------------------------------------------------------------
//  Template Implementation Below

template <typename... MElementTypes>
MTuple<MElementTypes...>::MTuple() {
  static_assert(
      (std::derived_from<
           MElementTypes,
           librarian::MVariable<MElementTypes,
                                typename MElementTypes::value_type>> &&
       ...),
      "The T1, T2, etc used in MTuple<T1, T2> must be a Moriarty variable. "
      "For example, MTuple<MInteger, MString>.");
  this->RegisterKnownProperty("size",
                              &MTuple<MElementTypes...>::OfSizeProperty);
}

template <typename... MElementTypes>
MTuple<MElementTypes...>::MTuple(MElementTypes... values)
    : elements_(values...) {
  static_assert(
      (std::derived_from<
           MElementTypes,
           librarian::MVariable<MElementTypes,
                                typename MElementTypes::value_type>> &&
       ...),
      "The T1, T2, etc used in MTuple<T1, T2> must be a Moriarty variable. "
      "For example, MTuple<MInteger, MString>.");
  this->RegisterKnownProperty("size",
                              &MTuple<MElementTypes...>::OfSizeProperty);
}

template <typename... MElementTypes>
std::string MTuple<MElementTypes...>::Typename() const {
  std::tuple typenames = std::apply(
      [](auto&&... elem) { return std::make_tuple(elem.Typename()...); },
      elements_);
  return absl::Substitute("MTuple<$0>", absl::StrJoin(typenames, ", "));
}

template <typename... MElementTypes>
MTuple<MElementTypes...>& MTuple<MElementTypes...>::WithSeparator(
    Whitespace separator) {
  if (separator_ && *separator_ != separator) {
    this->DeclareSelfAsInvalid(UnsatisfiedConstraintError(
        "Attempting to set multiple I/O separators for the same MTuple."));
    return *this;
  }
  separator_ = separator;
  return *this;
}

template <typename... MElementTypes>
template <int index, typename T>
MTuple<MElementTypes...>& MTuple<MElementTypes...>::Of(T variable) {
  static_assert(std::same_as<T, typename std::tuple_element_t<
                                    index, std::tuple<MElementTypes...>>>,
                "Parameter passed to MTuple::Of<> is the wrong type.");
  std::get<index>(elements_).MergeFrom(std::move(variable));
  return *this;
}

template <typename... MElementTypes>
absl::Status MTuple<MElementTypes...>::MergeFromImpl(
    const MTuple<MElementTypes...>& other) {
  absl::Status result;
  return this->MergeFromImpl(other,
                             std::index_sequence_for<MElementTypes...>());
}

template <typename... MElementTypes>
template <std::size_t... I>
absl::Status MTuple<MElementTypes...>::MergeFromImpl(
    const MTuple<MElementTypes...>& other, std::index_sequence<I...>) {
  absl::Status result;
  (result.Update(
       std::get<I>(elements_).TryMergeFrom(std::get<I>(other.elements_))),
   ...);
  return result;
}

template <typename... MElementTypes>
absl::StatusOr<typename MTuple<MElementTypes...>::tuple_value_type>
MTuple<MElementTypes...>::GenerateImpl() {
  return this->GenerateImpl(std::index_sequence_for<MElementTypes...>());
}

template <typename... MElementTypes>
template <std::size_t... I>
absl::StatusOr<typename MTuple<MElementTypes...>::tuple_value_type>
MTuple<MElementTypes...>::GenerateImpl(std::index_sequence<I...>) {
  absl::Status status;
  tuple_value_type result;
  (status.Update(GenerateSingleElement<I>(result)), ...);

  if (!status.ok()) return status;
  return result;
}

template <typename... MElementTypes>
template <std::size_t I>
absl::Status MTuple<MElementTypes...>::GenerateSingleElement(
    tuple_value_type& result) {
  MORIARTY_ASSIGN_OR_RETURN(
      std::get<I>(result),
      this->Random(absl::StrCat("element<", I, ">"), std::get<I>(elements_)));
  return absl::OkStatus();
}

template <typename... MElementTypes>
std::vector<std::string> MTuple<MElementTypes...>::GetDependenciesImpl() const {
  return GetDependenciesImpl(std::index_sequence_for<MElementTypes...>());
}

template <typename... MElementTypes>
template <std::size_t... I>
std::vector<std::string> MTuple<MElementTypes...>::GetDependenciesImpl(
    std::index_sequence<I...>) const {
  std::vector<std::string> dependencies;
  (absl::c_move(this->GetDependencies(std::get<I>(elements_)),
                std::back_inserter(dependencies)),
   ...);
  return dependencies;
}

template <typename... MElementTypes>
absl::Status MTuple<MElementTypes...>::PrintImpl(
    const tuple_value_type& value) {
  return PrintImpl(value, std::index_sequence_for<MElementTypes...>());
}

template <typename... MElementTypes>
template <std::size_t I>
absl::Status MTuple<MElementTypes...>::PrintElementWithLeadingSeparator(
    const tuple_value_type& value) {
  if (I > 0) {
    MORIARTY_ASSIGN_OR_RETURN(librarian::IOConfig * io_config,
                              this->GetIOConfig());
    MORIARTY_RETURN_IF_ERROR(io_config->PrintWhitespace(GetSeparator()));
  }

  return this->Print(absl::StrCat("element<", I, ">"), std::get<I>(elements_),
                     std::get<I>(value));
}

template <typename... MElementTypes>
template <std::size_t... I>
absl::Status MTuple<MElementTypes...>::PrintImpl(const tuple_value_type& value,
                                                 std::index_sequence<I...>) {
  absl::Status status;
  (status.Update(PrintElementWithLeadingSeparator<I>(value)), ...);
  return status;
}

template <typename... MElementTypes>
absl::Status MTuple<MElementTypes...>::IsSatisfiedWithImpl(
    const tuple_value_type& value) const {
  return IsSatisfiedWithImpl(value,
                             std::index_sequence_for<MElementTypes...>());
}

template <typename... MElementTypes>
template <std::size_t... I>
absl::Status MTuple<MElementTypes...>::IsSatisfiedWithImpl(
    const tuple_value_type& value, std::index_sequence<I...>) const {
  // TODO(b/208295758): This should be shortcircuited.
  absl::Status status;
  (status.Update(
       this->SatisfiesConstraints(std::get<I>(elements_), std::get<I>(value))),
   ...);

  return status;
}

template <typename... MElementTypes>
absl::StatusOr<typename MTuple<MElementTypes...>::tuple_value_type>
MTuple<MElementTypes...>::ReadImpl() {
  return ReadImpl(std::index_sequence_for<MElementTypes...>());
}

template <typename... MElementTypes>
template <std::size_t... I>
absl::StatusOr<typename MTuple<MElementTypes...>::tuple_value_type>
MTuple<MElementTypes...>::ReadImpl(std::index_sequence<I...>) {
  // TODO(b/208295758): This should be shortcircuited.
  absl::Status status;
  tuple_value_type result;
  (status.Update(TryReadAndSet<I>(result)), ...);
  if (!status.ok()) return status;
  return result;
}

template <typename... MElementTypes>
template <std::size_t I>
absl::Status MTuple<MElementTypes...>::TryReadAndSet(
    tuple_value_type& read_values) {
  if (I > 0) {
    MORIARTY_ASSIGN_OR_RETURN(librarian::IOConfig * io_config,
                              this->GetIOConfig());
    MORIARTY_RETURN_IF_ERROR(io_config->ReadWhitespace(GetSeparator()));
  }
  MORIARTY_ASSIGN_OR_RETURN(
      std::get<I>(read_values),
      this->Read(absl::StrCat("element<", I, ">"), std::get<I>(elements_)));
  return absl::OkStatus();
}

template <typename... MElementTypes>
Whitespace MTuple<MElementTypes...>::GetSeparator() const {
  return separator_.value_or(Whitespace::kSpace);
}

template <typename... MElementTypes>
absl::Status MTuple<MElementTypes...>::DistributePropertyToValues(
    Property property) {
  return DistributePropertyToValues(
      std::move(property), std::index_sequence_for<MElementTypes...>());
}

template <typename... MElementTypes>
template <std::size_t... I>
absl::Status MTuple<MElementTypes...>::DistributePropertyToValues(
    Property property, std::index_sequence<I...>) {
  absl::Status status;
  (status.Update(std::get<I>(elements_).TryWithKnownProperty(property)), ...);
  return status;
}

template <typename... MElementTypes>
absl::Status MTuple<MElementTypes...>::OfSizeProperty(Property property) {
  return DistributePropertyToValues(std::move(property));
}

}  // namespace moriarty

#endif  // MORIARTY_SRC_VARIABLES_MTUPLE_H_
