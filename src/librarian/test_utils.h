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

// MVariable test utilities
//
// In this file, you'll find several helpful functions when testing your custom
// Moriarty MVariable type.
//
// Generate Helpers:
//
//     * Generate(x, [optional] context)
//         Seeds an `MVariable` `x` with everything it needs, then generates a
//         value for it. If global context is needed (e.g., dependent
//         variables), then `context` provides that information.
//
//     * GenerateN(x, N, [optional] context)
//         Same as above, but generates N values. Prefer to use
//         `GeneratedValuesAre()` matcher below instead of generating lots of
//         values and checking each for a property.
//
//     * GenerateLots(x, [optional] context)
//         Calls `GenerateN(x, N)`, for some N (probably N=100 or 1000). Prefer
//         to use `GeneratedValuesAre()` matcher below instead of generating
//         lots of values and checking each for a property.
//
// Input / Output Helpers:
//
//    * Read(x, stream/string, [optional] context)
//        Reads a value using constraints from an MVariable `x` from the
//        input stream (or string) and returns that value.
//
//    * Print(x, value, [optional] context)
//        Prints `value` using constraints from an MVariable `x` to a string and
//        returns that string.
//
// Dependent Variables / Global Context
//
//    * Context
//        When variables depend on one another (e.g., length of an array is "N",
//        which is another MVariable), they need to live in the same Universe.
//        Context contains global information for several test functions.
//
// Matchers (for googletest):
//
//    * GeneratedValuesAre(M)
//        Generates several values from an MVariable and checks that each one
//        satisfies another googletest matcher M. Example:
//             EXPECT_THAT(MInteger().Between(1, 10), GeneratedValuesAre(Ge(1)))
//
//     * IsSatisfiedWith(x, [optional] context)
//         Determines if `x` satisfies the constraints of the variable. Example:
//             EXPECT_THAT(MInteger().Between(1, 10), IsSatisfiedWith(6))
//
//     * IsNotSatisfiedWith(x, reason, [optional] context)
//         Determines if `x` does not satisfy the constraints of the variable.
//         Prefer this over Not(IsSatisfiedWith()).
//         Example:
//             EXPECT_THAT(MInteger().Between(1, 10),
//                         IsNotSatisfiedWith(60, "range"))
//
// Other googletest helpers:
//
//    * GenerateSameValues(a, b)
//        Checks that MVariables `a` and `b` generate the same stream of values.
//             EXPECT_TRUE(GenerateSameValues(MInteger().Between(1, 10),
//                                            MInteger().AtLeast(1).AtMost(1)));
//
//    * AllGenerateSameValues({a, b, c, d})
//        Same as GenerateSameValues, except checks that all variables generate
//        the same values.

#ifndef MORIARTY_SRC_LIBRARIAN_TEST_UTILS_H_
#define MORIARTY_SRC_LIBRARIAN_TEST_UTILS_H_

#include <any>
#include <concepts>
#include <istream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/algorithm/container.h"
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/errors.h"
#include "src/internal/abstract_variable.h"
#include "src/internal/analysis_bootstrap.h"
#include "src/internal/generation_bootstrap.h"
#include "src/internal/generation_config.h"
#include "src/internal/random_engine.h"
#include "src/internal/universe.h"
#include "src/internal/value_set.h"
#include "src/internal/variable_set.h"
#include "src/librarian/io_config.h"
#include "src/librarian/mvariable.h"
#include "src/util/status_macro/status_macros.h"

namespace moriarty_testing {

// TODO(darcybest): Add tests that test this test_utils file.

namespace moriarty_testing_internal {
class ContextManager;  // Forward declaring internal extended API.
}  // namespace moriarty_testing_internal

class Context {
 public:
  // WithValue()
  //
  // Sets a known value in the global context.
  template <typename T>
    requires std::derived_from<
        T, moriarty::librarian::MVariable<T, typename T::value_type>>
  Context& WithValue(absl::string_view variable_name, T::value_type value) {
    values_.Set<T>(variable_name, value);
    return *this;
  }

  // WithVariable()
  //
  // Sets a variable in the global context.
  template <typename T>
    requires std::derived_from<
        T, moriarty::librarian::MVariable<T, typename T::value_type>>
  Context& WithVariable(absl::string_view variable_name, T variable) {
    ABSL_CHECK_OK(variables_.AddVariable(variable_name, variable));
    return *this;
  }

 private:
  friend class moriarty_testing_internal::ContextManager;  // Internal Extended
                                                           // API

  moriarty::moriarty_internal::VariableSet variables_;
  moriarty::moriarty_internal::ValueSet values_;
};

// Generate() [For tests only]
//
// Generates a value for `variable`. If external context is needed (e.g., for
// dependent variables), then `context` provides that information.
//
// Example:
//   EXPECT_THAT(Generate(MInteger()), IsOkAndHolds(5));
//   EXPECT_THAT(Generate(MString().OfLength(10)), IsOkAndHolds(SizeIs(10)));
//   EXPECT_THAT(Generate(MInteger().Is("3 * N + 1"),
//                        Context().WithValue("N", 3)),
//               IsOkAndHolds(10));

template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Generate(T variable,
                                                Context context = {});

// GenerateN() [For tests only]
//
// Generates N values from `variable`. If external context is needed (e.g.,
// dependent variables), then `context` provides that information.
//
// Note: `context` may (and likely will) be modified. Including values stored.
//
// Examples: Each example generates 50 items.
//   EXPECT_THAT(GenerateN(MInteger().AtLeast(2), 50),
//               IsOkAndHolds(Each(Ge(2))));
//   EXPECT_THAT(GenerateN(MString().OfLength(10), 50),
//               IsOkAndHolds(Each(SizeIs(10))));
//   EXPECT_THAT(Generate(MInteger().Between(1, "3 * N + 1"), 50,
//                        Context().WithValue("N", 3)),
//               IsOkAndHolds(Each(AllOf(Ge(1), Le(10)));
template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<std::vector<typename T::value_type>> GenerateN(
    T variable, int N, Context context = {});

// GenerateLots() [For tests only]
//
// NOTE: 100 may change in the future, do not depend on this exact value. If
// you care about the exact number, use `GenerateN` instead.
//
// Generates 100 values from `variable` after seeding the variable with all
// needed information. Convenience wrapper for `GenerateN(var, 100)` so the
// magic number 100 doesn't appear all over tests.
template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<std::vector<typename T::value_type>> GenerateLots(
    T variable, Context context = {});

// GetUniqueValue() [For tests only]
//
// Returns the unique value for this variable.
template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
std::optional<typename T::value_type> GetUniqueValue(T variable,
                                                     Context context = {});

// GenerateDifficultInstancesValues() [For tests only]
//
// Generates values for all the merged `variable` instances obtained from
// `MVariable::GetDifficultInstances` for the specific type provided.
template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<std::vector<typename T::value_type>>
GenerateDifficultInstancesValues(T variable);

// Read() [For tests only]
//
// Reads a value from the input stream and returns that value. (see version
// below for a string input)
template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Read(T variable, std::istream& is,
                                            Context context = {});

// Read() [For tests only]
//
// Reads a value from the string and returns that value. (see version
// below for an input stream, especially if the state of the stream is important
// after the read)
template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Read(T variable,
                                            absl::string_view read_from,
                                            Context context = {});

// Print() [For tests only]
//
// Prints `value` using constraints from an MVariable `x` to a string and
// returns that string.
template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<std::string> Print(T variable, typename T::value_type value,
                                  Context context = {});

// GenerateSameValues() [for use with GoogleTest]
//
// Determines if two MVariables generate the same stream of values.
//
// Example usage:
//
//  EXPECT_TRUE(GenerateSameValues(MCustomType().WithSomeFunConstraint(),
//                                 MCustomType().WithSomeOtherConstraint()));
//
// How it works? (do not depend on this behavior described below)
// Seeds both variables with the same random seed, then generates several values
// and checks that the same values are generated in the same order.
template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
testing::AssertionResult GenerateSameValues(T a, T b);

// AllGenerateSameValues() [for use with GoogleTest]
//
// Determines if several MVariables generate the same stream of values.
//
// Example usage:
//
//  EXPECT_TRUE(AllGenerateSameValues<MCustomType>(
//      {MCustomType().WithSomeFunConstraint(),
//       MCustomType().WithSomeOtherConstraint(),
//       MCustomType().AgainSomething()}));
//
// How it works? (do not depend on this behavior described below)
// Seeds all variables with the same random seed, then generates several values
// and checks that the same values are generated in the same order.
template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
testing::AssertionResult AllGenerateSameValues(std::vector<T> vars);

// GeneratedValuesAre() [for use with GoogleTest]
//
// Determines if values generated from this variable satisfy a constraint.
//
// Example usage:
//
//  EXPECT_THAT(MInteger().Between(1, 10), GeneratedValuesAre(Ge(1)));
//
// How it works? (do not depend on this behavior described below)
// Generates several values from the variable and checks that all of them
// satisfy the given matcher.
MATCHER_P(GeneratedValuesAre, matcher,
          absl::Substitute("generated values $0 satisfy constraints",
                           negation ? "should not" : "should")) {
  // The `auto` is hiding an absl::StatusOr<std::vector<>> of the generated type
  auto values = GenerateLots(arg);
  if (!values.ok()) {
    *result_listener << "Failed to generate values. " << values.status();
    return false;
  }

  for (int i = 0; i < values->size(); i++) {
    auto value = (*values)[i];  // `auto` is hiding the generated type
    if (!testing::ExplainMatchResult(matcher, value, result_listener)) {
      *result_listener << "The " << i + 1 << "-th generated value ("
                       << testing::PrintToString(value)
                       << ") does not satisfy matcher; ";
      return false;
    }
  }

  *result_listener << "all generated values satisfy constraints";
  return true;
}

// -----------------------------------------------------------------------------
//  ContextManager

namespace moriarty_testing_internal {

class ContextManager {
 public:
  explicit ContextManager(Context* context) : managed_context_(context) {}

  moriarty::moriarty_internal::VariableSet* GetVariables() {
    return &managed_context_->variables_;
  }

  moriarty::moriarty_internal::ValueSet* GetValues() {
    return &managed_context_->values_;
  }

 private:
  Context* managed_context_;
};

}  // namespace moriarty_testing_internal

template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Generate(T variable, Context context) {
  std::string var_name = absl::Substitute("Generate($0)", variable.Typename());
  context.WithVariable(var_name, variable);

  moriarty::moriarty_internal::RandomEngine rng({3, 4, 5}, "");
  moriarty::moriarty_internal::GenerationConfig generation_config;
  moriarty_testing_internal::ContextManager manager(&context);

  MORIARTY_ASSIGN_OR_RETURN(
      moriarty::moriarty_internal::ValueSet values,
      moriarty::moriarty_internal::GenerateAllValues(
          *manager.GetVariables(), *manager.GetValues(),
          {rng, /* soft_generation_limit = */ std::nullopt}));

  return values.Get<T>(var_name);
}

template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<std::vector<typename T::value_type>> GenerateN(T variable, int N,
                                                              Context context) {
  std::string var_name = absl::Substitute("GenerateN($0)", variable.Typename());
  context.WithVariable(var_name, variable);

  moriarty::moriarty_internal::RandomEngine rng({3, 4, 5}, "");

  std::vector<typename T::value_type> res;
  res.reserve(N);
  for (int i = 0; i < N; i++) {
    // We need to make a copy so that we don't set values for future `i` values.
    Context context_copy = context;
    moriarty_testing_internal::ContextManager manager(&context_copy);
    MORIARTY_ASSIGN_OR_RETURN(
        moriarty::moriarty_internal::ValueSet values,
        moriarty::moriarty_internal::GenerateAllValues(
            *manager.GetVariables(), *manager.GetValues(), {rng}));
    MORIARTY_ASSIGN_OR_RETURN(typename T::value_type value,
                              values.Get<T>(var_name));
    res.push_back(std::move(value));
  }
  return res;
}

template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<std::vector<typename T::value_type>> GenerateLots(
    T variable, Context context) {
  return GenerateN(variable, 100, std::move(context));
}

template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
std::optional<typename T::value_type> GetUniqueValue(T variable,
                                                     Context context) {
  std::string var_name =
      absl::Substitute("GetUniqueValue($0)", variable.Typename());
  context.WithVariable(var_name, variable);
  moriarty_testing_internal::ContextManager manager(&context);

  moriarty::moriarty_internal::Universe universe =
      moriarty::moriarty_internal::Universe()
          .SetMutableVariableSet(manager.GetVariables())
          .SetMutableValueSet(manager.GetValues());
  manager.GetVariables()->SetUniverse(&universe);

  // The next line hides a *StatusOr<>. Should crash if not okay.
  moriarty::moriarty_internal::AbstractVariable* var =
      *manager.GetVariables()->GetAbstractVariable(var_name);
  std::optional<std::any> value = var->GetUniqueValueUntyped();
  if (!value) return std::nullopt;
  return std::any_cast<typename T::value_type>(*value);
}

template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Read(T variable, std::istream& is,
                                            Context context) {
  std::string var_name = absl::Substitute("Read($0)", variable.Typename());
  context.WithVariable(var_name, variable);
  moriarty_testing_internal::ContextManager manager(&context);

  moriarty::librarian::IOConfig io_config;
  io_config.SetInputStream(is);

  moriarty::moriarty_internal::Universe universe =
      moriarty::moriarty_internal::Universe()
          .SetMutableVariableSet(manager.GetVariables())
          .SetMutableValueSet(manager.GetValues())
          .SetIOConfig(&io_config);
  manager.GetVariables()->SetUniverse(&universe);

  MORIARTY_ASSIGN_OR_RETURN(
      moriarty::moriarty_internal::AbstractVariable * var,
      manager.GetVariables()->GetAbstractVariable(var_name));
  MORIARTY_RETURN_IF_ERROR(var->ReadValue());
  return manager.GetValues()->Get<T>(var_name);
}

template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<typename T::value_type> Read(T variable,
                                            absl::string_view read_from,
                                            Context context) {
  std::istringstream ss((std::string(read_from)));
  return Read(variable, ss, std::move(context));
}

template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<std::string> Print(T variable, typename T::value_type value,
                                  Context context) {
  std::string var_name = absl::Substitute("Print($0)", variable.Typename());
  context.WithVariable(var_name, variable);
  context.WithValue<T>(var_name, value);
  moriarty_testing_internal::ContextManager manager(&context);

  moriarty::librarian::IOConfig io_config;
  std::ostringstream ss;
  io_config.SetOutputStream(ss);

  moriarty::moriarty_internal::Universe universe =
      moriarty::moriarty_internal::Universe()
          .SetMutableVariableSet(manager.GetVariables())
          .SetMutableValueSet(manager.GetValues())
          .SetIOConfig(&io_config);
  manager.GetVariables()->SetUniverse(&universe);

  MORIARTY_ASSIGN_OR_RETURN(
      moriarty::moriarty_internal::AbstractVariable * var,
      manager.GetVariables()->GetAbstractVariable(var_name));
  MORIARTY_RETURN_IF_ERROR(var->PrintValue());
  return ss.str();
}

template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
testing::AssertionResult GenerateSameValues(T a, T b) {
  // The `auto` is hiding an absl::StatusOr<std::vector<>> of the generated type
  auto a_values = GenerateLots(a);
  if (!a_values.ok()) {
    return testing::AssertionFailure()
           << "First parameter failed to generate values. "
           << a_values.status();
  }
  auto b_values = GenerateLots(b);
  if (!b_values.ok()) {
    return testing::AssertionFailure()
           << "Second parameter failed to generate values. "
           << b_values.status();
  }

  ABSL_CHECK_EQ(a_values->size(), b_values->size())
      << "GenerateLots() should generate the same number of values every time.";

  auto [it_a, it_b] = absl::c_mismatch(*a_values, *b_values);
  if (it_a != a_values->end()) {
    return testing::AssertionFailure()
           << "the two variables generate different values. Example: "
           << testing::PrintToString(*it_a) << " vs "
           << testing::PrintToString(*it_b) << " (found after generating "
           << std::distance(a_values->begin(), it_a) + 1 << " value(s) each)";
  }

  return testing::AssertionSuccess()
         << "all " << a_values->size() << " generated values match";
}

template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
testing::AssertionResult AllGenerateSameValues(std::vector<T> vars) {
  if (vars.empty()) {
    return testing::AssertionFailure()
           << "You probably didn't mean to pass an empty array?";
  }

  for (int i = 1; i < vars.size(); i++) {
    if (testing::AssertionResult result = GenerateSameValues(vars[0], vars[i]);
        !result) {
      return testing::AssertionFailure()
             << "variables at index 0 and index " << i << " do not match; "
             << result.message();
    }
  }

  return testing::AssertionSuccess() << "all generated values match";
}

template <typename T>
  requires std::derived_from<
      T, moriarty::librarian::MVariable<T, typename T::value_type>>
absl::StatusOr<std::vector<typename T::value_type>>
GenerateDifficultInstancesValues(T variable) {
  MORIARTY_ASSIGN_OR_RETURN(std::vector<T> instances,
                            variable.GetDifficultInstances());

  std::vector<typename T::value_type> values;
  for (T difficult_variable : instances) {
    MORIARTY_ASSIGN_OR_RETURN(typename T::value_type val,
                              Generate(std::move(difficult_variable)));
    values.push_back(std::move(val));
  }
  return values;
}

// IsSatisfiedWith() [for use with GoogleTest]
//
// Determines if values generated from this variable satisfy a constraint.
// Also see `IsNotSatisfiedWith`.
//
// Example usage:
//
//  EXPECT_THAT(MInteger().Between(1, 10), IsSatisfiedWith(6))
//  EXPECT_THAT(MInteger().AtMost("N"),
//              IsSatisfiedWith(5, Context().WithValue("N", 10)));
MATCHER_P2(IsSatisfiedWith, value, context, "") {
  // We cast `value` to the appropriate generated type for the test. So
  // int -> int64_t, char* -> std::string, etc.
  using ValueType = typename std::decay_t<arg_type>::value_type;
  // context is const& in MATCHERs, we need a non-const.
  Context context_copy = context;
  moriarty_testing_internal::ContextManager manager(&context_copy);

  std::string var_name =
      absl::Substitute("$0::IsSatisfiedWith", arg.Typename());
  absl::Status satisfies = moriarty::moriarty_internal::SatisfiesConstraints(
      arg, ValueType(value), *manager.GetValues(), *manager.GetVariables(),
      var_name);

  if (moriarty::IsUnsatisfiedConstraintError(satisfies)) {
    *result_listener << "value does not satisfy constraints: "
                     << satisfies.message();
    return false;
  }

  if (!satisfies.ok()) {
    *result_listener << "Non-Moriarty error: " << satisfies.message();
    return false;
  }

  *result_listener << "value satisfies constraints";
  return true;
}

MATCHER_P(IsSatisfiedWith, value, "") {
  using ValueType = typename std::decay_t<arg_type>::value_type;
  return testing::ExplainMatchResult(
      IsSatisfiedWith(ValueType(value), Context()), arg, result_listener);
}

// IsNotSatisfiedWith() [for use with GoogleTest]
//
// Determines if values generated from this variable satisfy a constraint.
// Also see `IsSatisfiedWith`. This will check that `reason` is a substring of
// the Moriarty error message produced.
//
// Example usage:
//
//  EXPECT_THAT(MInteger().Between(1, 10), IsNotSatisfiedWith(0, "range"))
//  EXPECT_THAT(MInteger().AtMost("N"),
//              IsNotSatisfiedWith(50, "range", Context().WithValue("N",
//              10)));
MATCHER_P3(IsNotSatisfiedWith, value, reason, context, "") {
  // We cast `value` to the appropriate generated type for the test. So
  // int -> int64_t, char* -> std::string, etc.
  using ValueType = typename std::decay_t<arg_type>::value_type;
  // context is const& in MATCHERs, we need a non-const.
  Context context_copy = context;
  moriarty_testing_internal::ContextManager manager(&context_copy);

  std::string var_name =
      absl::Substitute("$0::IsNotSatisfiedWith", arg.Typename());
  absl::Status satisfies = moriarty::moriarty_internal::SatisfiesConstraints(
      arg, ValueType(value), *manager.GetValues(), *manager.GetVariables(),
      var_name);

  if (moriarty::IsUnsatisfiedConstraintError(satisfies)) {
    if (!testing::Value(satisfies.message(), testing::HasSubstr(reason))) {
      *result_listener << "value does not satisfy constraints, but not for the "
                          "correct reason. Expected '"
                       << reason << "', got '" << satisfies.message() << "'";
      return false;
    }
    *result_listener << "value does not satisfy constraints: "
                     << satisfies.message();
    return true;
  }

  if (!satisfies.ok()) {
    *result_listener << "non-Moriarty error: " << satisfies.message();
    return false;
  }

  *result_listener << "value satisfies constraints";
  return false;
}

MATCHER_P2(IsNotSatisfiedWith, value, reason, "") {
  using ValueType = typename std::decay_t<arg_type>::value_type;
  return testing::ExplainMatchResult(
      IsNotSatisfiedWith(ValueType(value), reason, Context()), arg,
      result_listener);
}

}  // namespace moriarty_testing

#endif  // MORIARTY_SRC_LIBRARIAN_TEST_UTILS_H_
