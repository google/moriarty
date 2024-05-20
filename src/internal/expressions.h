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

#ifndef MORIARTY_SRC_INTERNAL_EXPRESSIONS_H_
#define MORIARTY_SRC_INTERNAL_EXPRESSIONS_H_

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

namespace moriarty {

class Expression;  // Forward declaring Expression.

// EvaluateIntegerExpression()
//
// Evaluates an expression tree of binary/unary operations and returns the
// integer response. If any intermediate step in the calculation overflows a
// 64-bit signed integer, a FailedPreconditionError is thrown.
// If there are variables present in `expression`, then `variables` will be
// used as their values. All variables must be present in `variables`.
absl::StatusOr<int64_t> EvaluateIntegerExpression(const Expression& expression);
absl::StatusOr<int64_t> EvaluateIntegerExpression(
    const Expression& expression,
    const absl::flat_hash_map<std::string, int64_t>& variables);

// ParseExpression()
//
// Given a string representation in infix notation, returns the corresponding
// `Expression`.
absl::StatusOr<Expression> ParseExpression(absl::string_view expression);

// NeededVariables()
//
// Returns all variables needed in order to evaluate `expression`.
absl::StatusOr<absl::flat_hash_set<std::string>> NeededVariables(
    const Expression& expression);

namespace moriarty_internal {

enum class BinaryOperator {
  // Input: (int, int) Output: int
  kAdd,
  kSubtract,
  kMultiply,
  kDivide,
  kModulo,
  kExponentiate
};

enum class UnaryOperator { kPlus, kNegate };

class BinaryOperation {
 public:
  BinaryOperation(Expression lhs, BinaryOperator op, Expression rhs);
  BinaryOperation(const BinaryOperation& other);
  BinaryOperation& operator=(BinaryOperation other);
  BinaryOperation(BinaryOperation&& other) noexcept = default;

  BinaryOperator Op() const { return op_; }
  const Expression& Lhs() const { return *lhs_; }
  const Expression& Rhs() const { return *rhs_; }

 private:
  BinaryOperator op_;
  std::unique_ptr<Expression> lhs_;
  std::unique_ptr<Expression> rhs_;
};

class UnaryOperation {
 public:
  UnaryOperation(UnaryOperator op, Expression rhs);
  UnaryOperation(const UnaryOperation& other);
  UnaryOperation& operator=(UnaryOperation other);
  UnaryOperation(UnaryOperation&& other) noexcept = default;

  UnaryOperator Op() const { return op_; }
  const Expression& Rhs() const { return *rhs_; }

 private:
  UnaryOperator op_;
  std::unique_ptr<Expression> rhs_;
};

class Function {
 public:
  Function(std::string name,
           std::vector<std::unique_ptr<Expression>> arguments);
  Function(const Function& other);
  Function& operator=(Function other);
  Function(Function&& other) noexcept = default;

  std::string Name() const { return name_; }
  const std::vector<std::unique_ptr<Expression>>& Arguments() const {
    return arguments_;
  }

  void AppendArgument(Expression expr);
  void ReverseArguments() { absl::c_reverse(arguments_); }
  void SetName(std::string name) { name_ = std::move(name); }

 private:
  std::string name_;
  std::vector<std::unique_ptr<Expression>> arguments_;
};

class Literal {
 public:
  explicit Literal(int64_t value) : value_(value) {}
  explicit Literal(std::string value) : value_(std::move(value)) {}

  bool IsVariable() const {
    return std::holds_alternative<std::string>(value_);
  }
  // Precondition: IsVariable() == true
  std::string VariableName() const { return std::get<std::string>(value_); }
  // Precondition: IsVariable() == false
  int64_t Value() const { return std::get<int64_t>(value_); }

 private:
  std::variant<std::string, int64_t> value_;
};

}  // namespace moriarty_internal

class Expression {
 public:
  using Types = std::variant<
      moriarty_internal::Literal, moriarty_internal::BinaryOperation,
      moriarty_internal::UnaryOperation, moriarty_internal::Function>;

  // Constructors are not `explicit` here since each of these objects *are*
  // `Expression`s.
  Expression(moriarty_internal::Literal literal);            // NOLINT
  Expression(moriarty_internal::BinaryOperation binary_op);  // NOLINT
  Expression(moriarty_internal::UnaryOperation unary_op);    // NOLINT
  Expression(moriarty_internal::Function function);          // NOLINT

  const Types& Get() const { return expression_; }

  void SetString(absl::string_view str) { str_expression_ = std::string(str); }
  [[nodiscard]] std::string ToString() const { return str_expression_; }

 private:
  Types expression_;
  std::string str_expression_;
};

}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_EXPRESSIONS_H_
