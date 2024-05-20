// Copyright 2024 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/internal/expressions.h"

#include <cstdint>
#include <iomanip>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "src/util/status_macro/status_macros.h"
#include "src/util/test_status_macro/status_testutil.h"

namespace moriarty {
namespace {

using ::moriarty::IsOk;
using ::moriarty::IsOkAndHolds;
using ::moriarty::StatusIs;
using ::moriarty::moriarty_internal::BinaryOperation;
using ::moriarty::moriarty_internal::BinaryOperator;
using ::moriarty::moriarty_internal::Function;
using ::moriarty::moriarty_internal::Literal;
using ::moriarty::moriarty_internal::UnaryOperation;
using ::moriarty::moriarty_internal::UnaryOperator;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Pointee;
using ::testing::UnorderedElementsAre;
using ::testing::VariantWith;

Expression SimpleBinaryOp(int64_t lhs, BinaryOperator op, int64_t rhs) {
  return BinaryOperation(Literal(lhs), op, Literal(rhs));
}

Expression SimpleUnaryOp(UnaryOperator op, int64_t rhs) {
  return UnaryOperation(op, Literal(rhs));
}

MATCHER(IsOverflow, "Overflow should occur and throw FailedPreconditionError") {
  return testing::ExplainMatchResult(
      StatusIs(absl::StatusCode::kFailedPrecondition,
               testing::ContainsRegex("(o|O)verflow")),
      arg, result_listener);
}

/* -------------------------------------------------------------------------- */
/*  BASIC TESTS                                                               */
/* -------------------------------------------------------------------------- */

TEST(ExpressionsTest, SingletonLiteralShouldReturnThatValue) {
  EXPECT_THAT(EvaluateIntegerExpression(Literal(10)), IsOkAndHolds(10));
  EXPECT_THAT(EvaluateIntegerExpression(Literal(0)), IsOkAndHolds(0));
  EXPECT_THAT(EvaluateIntegerExpression(Literal(-117)), IsOkAndHolds(-117));
}

TEST(ExpressionsTest, SimpleUnaryOpsShouldReturnExpectedValue) {
  EXPECT_THAT(EvaluateIntegerExpression(SimpleUnaryOp(UnaryOperator::kPlus, 3)),
              IsOkAndHolds(3));
  EXPECT_THAT(EvaluateIntegerExpression(SimpleUnaryOp(UnaryOperator::kPlus, 0)),
              IsOkAndHolds(0));
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleUnaryOp(UnaryOperator::kNegate, 0)),
      IsOkAndHolds(0));
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleUnaryOp(UnaryOperator::kNegate, 3)),
      IsOkAndHolds(-3));
}

TEST(ExpressionsTest, SimpleBinaryOpsShouldReturnExpectedValue) {
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(10, BinaryOperator::kAdd, 3)),
      IsOkAndHolds(10 + 3));
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(-5, BinaryOperator::kAdd, 0)),
      IsOkAndHolds(-5 + 0));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(10, BinaryOperator::kSubtract, 3)),
              IsOkAndHolds(10 - 3));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-5, BinaryOperator::kSubtract, 0)),
              IsOkAndHolds(-5 - 0));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-5, BinaryOperator::kMultiply, 3)),
              IsOkAndHolds(-5 * 3));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-5, BinaryOperator::kMultiply, 0)),
              IsOkAndHolds(-5 * 0));
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(5, BinaryOperator::kDivide, -3)),
      IsOkAndHolds(5 / -3));
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(20, BinaryOperator::kDivide, 4)),
      IsOkAndHolds(20 / 4));
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(5, BinaryOperator::kModulo, -3)),
      IsOkAndHolds(5 % -3));
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(20, BinaryOperator::kModulo, 4)),
      IsOkAndHolds(20 % 4));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(3, BinaryOperator::kExponentiate, 4)),
              IsOkAndHolds(81));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(2, BinaryOperator::kExponentiate, 10)),
              IsOkAndHolds(1024));
}

TEST(ExpressionsTest, ExponentiationOfZeroOrOneShouldReturnZeroOrOne) {
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(0, BinaryOperator::kExponentiate, 1)),
              IsOkAndHolds(0));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(0, BinaryOperator::kExponentiate, 1000000000)),
              IsOkAndHolds(0));

  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(1, BinaryOperator::kExponentiate, 0)),
              IsOkAndHolds(1));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(1, BinaryOperator::kExponentiate, 1)),
              IsOkAndHolds(1));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(1, BinaryOperator::kExponentiate, 1000000000)),
              IsOkAndHolds(1));

  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-1, BinaryOperator::kExponentiate, 0)),
              IsOkAndHolds(1));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-1, BinaryOperator::kExponentiate, 1)),
              IsOkAndHolds(-1));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-1, BinaryOperator::kExponentiate, 999999999)),
              IsOkAndHolds(-1));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-1, BinaryOperator::kExponentiate, 100000000)),
              IsOkAndHolds(1));
}

TEST(ExpressionsTest, AnythingNonzeroRaisedToZeroShouldReturnOne) {
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(3, BinaryOperator::kExponentiate, 0)),
              IsOkAndHolds(1));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-2, BinaryOperator::kExponentiate, 0)),
              IsOkAndHolds(1));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(12356869, BinaryOperator::kExponentiate, 0)),
              IsOkAndHolds(1));
}

/* -------------------------------------------------------------------------- */
/*  NESTED EXPRESSION TESTS                                                   */
/* -------------------------------------------------------------------------- */

TEST(ExpressionsTest, NestingExpressionsShouldWork) {
  EXPECT_THAT(EvaluateIntegerExpression(BinaryOperation(
                  Literal(3), BinaryOperator::kSubtract,
                  BinaryOperation(Literal(5), BinaryOperator::kAdd,
                                  SimpleUnaryOp(UnaryOperator::kNegate, 7)))),
              IsOkAndHolds(3 - (5 + (-7))));

  EXPECT_THAT(
      EvaluateIntegerExpression(BinaryOperation(
          BinaryOperation(Literal(5), BinaryOperator::kAdd,
                          UnaryOperation(UnaryOperator::kPlus, Literal(7))),
          BinaryOperator::kSubtract, Literal(3))),
      IsOkAndHolds((5 + (+7)) - 3));
}

TEST(ExpressionsTest,
     ErrorsInNestedExpressionShouldTriggerEvenThoughEntireAnswerFitsIn64Bits) {
  absl::StatusOr<Expression> expr = ParseExpression(
      absl::Substitute("($0 + $0) - $0", std::numeric_limits<int64_t>::max()));

  MORIARTY_ASSERT_OK(expr);
  EXPECT_THAT(EvaluateIntegerExpression(*expr), IsOverflow());
}

/* -------------------------------------------------------------------------- */
/*  OVERFLOW TESTS                                                            */
/* -------------------------------------------------------------------------- */

TEST(ExpressionsTest, NegatingSmallestIntegerShouldFail) {
  EXPECT_THAT(EvaluateIntegerExpression(SimpleUnaryOp(
                  UnaryOperator::kNegate, std::numeric_limits<int64_t>::min())),
              IsOverflow());
}

TEST(ExpressionsTest, AdditionOverflowShouldFail) {
  int64_t maxi = std::numeric_limits<int64_t>::max();
  int64_t mini = std::numeric_limits<int64_t>::min();

  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(maxi, BinaryOperator::kAdd, 1)),
      IsOverflow());
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(1, BinaryOperator::kAdd, maxi)),
      IsOverflow());
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(mini, BinaryOperator::kAdd, -1)),
      IsOverflow());
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(-1, BinaryOperator::kAdd, mini)),
      IsOverflow());
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(maxi, BinaryOperator::kAdd, mini)),
              IsOkAndHolds(-1));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(mini, BinaryOperator::kAdd, maxi)),
              IsOkAndHolds(-1));
}

TEST(ExpressionsTest, SubtractionOverflowShouldFail) {
  int64_t maxi = std::numeric_limits<int64_t>::max();
  int64_t mini = std::numeric_limits<int64_t>::min();
  int64_t large = maxi / 2 + 100;

  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-large, BinaryOperator::kSubtract, large)),
              IsOverflow());
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(mini, BinaryOperator::kSubtract, 1)),
              IsOverflow());
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(maxi, BinaryOperator::kSubtract, -1)),
              IsOverflow());
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(maxi, BinaryOperator::kSubtract, mini)),
              IsOverflow());
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(mini, BinaryOperator::kSubtract, maxi)),
              IsOverflow());
}

TEST(ExpressionsTest, MultiplicationOverflowShouldFail) {
  int64_t maxi = std::numeric_limits<int64_t>::max();
  int64_t mini = std::numeric_limits<int64_t>::min();
  int64_t two31 = (1ll << 31);
  int64_t two32 = (1ll << 32);

  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(mini, BinaryOperator::kMultiply, -1)),
              IsOverflow());
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-1, BinaryOperator::kMultiply, mini)),
              IsOverflow());

  // Ensure that -2^63 isn't flagged as overflow
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(mini, BinaryOperator::kMultiply, 1)),
              IsOkAndHolds(mini));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(1, BinaryOperator::kMultiply, mini)),
              IsOkAndHolds(mini));

  // Boundary cases.
  // Note that maxi is divisible by 49. mini-1 is divisible by 19.
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(two31, BinaryOperator::kMultiply, two32)),
              IsOverflow());
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(49, BinaryOperator::kMultiply, maxi / 49)),
              IsOkAndHolds(maxi));
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-two32, BinaryOperator::kMultiply, two31)),
              IsOkAndHolds(mini));
  EXPECT_THAT(EvaluateIntegerExpression(SimpleBinaryOp(
                  513, BinaryOperator::kMultiply, 17979282722913793LL)),
              IsOverflow());  // -2^63 - 1

  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(mini, BinaryOperator::kMultiply, 5)),
              IsOverflow());
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(7, BinaryOperator::kMultiply, maxi - two32)),
              IsOverflow());
}

TEST(ExpressionsTest, ExponentiationOverflowShouldFail) {
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(2, BinaryOperator::kExponentiate, 63)),
              IsOverflow());
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-2, BinaryOperator::kExponentiate, 64)),
              IsOverflow());
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(-2, BinaryOperator::kExponentiate, 63)),
              IsOk());
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(123, BinaryOperator::kExponentiate, 45678)),
              IsOverflow());
}

/* -------------------------------------------------------------------------- */
/*  INVALID OPERATIONS                                                        */
/* -------------------------------------------------------------------------- */

TEST(ExpressionsTest, DivisionByZeroShouldThrowErrorStatus) {
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(10, BinaryOperator::kDivide, 0)),
      StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(0, BinaryOperator::kDivide, 0)),
      StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(0, BinaryOperator::kDivide, 10)),
      IsOk());
}

TEST(ExpressionsTest, ModuloByZeroShouldThrowErrorStatus) {
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(10, BinaryOperator::kModulo, 0)),
      StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(0, BinaryOperator::kModulo, 0)),
      StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(
      EvaluateIntegerExpression(SimpleBinaryOp(0, BinaryOperator::kModulo, 10)),
      IsOk());
}

TEST(ExpressionsTest, ZeroRaisedToZeroShouldReturnError) {
  EXPECT_THAT(EvaluateIntegerExpression(
                  SimpleBinaryOp(0, BinaryOperator::kExponentiate, 0)),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

/* -------------------------------------------------------------------------- */
/*  UNKNOWN VARIABLES                                                         */
/* -------------------------------------------------------------------------- */

TEST(ExpressionsTest, UnknownVariablesShouldFail) {
  EXPECT_THAT(EvaluateIntegerExpression(Literal("N")),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(
      EvaluateIntegerExpression(
          BinaryOperation(Literal(3), BinaryOperator::kAdd, Literal("N"))),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("unknown value")));
  EXPECT_THAT(
      EvaluateIntegerExpression(
          UnaryOperation(UnaryOperator::kPlus, Literal("N"))),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("unknown value")));
}

/* -------------------------------------------------------------------------- */
/*  STRING PARSING                                                            */
/* -------------------------------------------------------------------------- */

[[nodiscard]] testing::AssertionResult EvaluateAndCheck(
    absl::string_view expression, int64_t expected) {
  absl::StatusOr<Expression> exp = ParseExpression(expression);
  if (!exp.ok())
    return testing::AssertionFailure()
           << std::quoted(expression) << " failed to parse; " << exp.status();

  absl::StatusOr<int64_t> val = EvaluateIntegerExpression(*exp);
  if (!val.ok())
    return testing::AssertionFailure()
           << std::quoted(expression)
           << " failed to evaluate after successfully parsing; "
           << val.status();

  if (*val != expected)
    return testing::AssertionFailure()
           << std::quoted(expression) << " evaluated to " << *val;
  return testing::AssertionSuccess();
}

TEST(ExpressionsTest, SingleNonnegativeIntegersWork) {
  EXPECT_TRUE(EvaluateAndCheck("0", 0));
  EXPECT_TRUE(EvaluateAndCheck("3", 3));
  EXPECT_TRUE(EvaluateAndCheck("123456", 123456));
  EXPECT_TRUE(
      EvaluateAndCheck(std::to_string(std::numeric_limits<int64_t>::max()),
                       std::numeric_limits<int64_t>::max()));
  EXPECT_THAT(ParseExpression("12345678901234567890"), IsOverflow());
}

TEST(ExpressionsTest, SingleNegativeIntegersWork) {
  EXPECT_TRUE(EvaluateAndCheck("-0", 0));
  EXPECT_TRUE(EvaluateAndCheck("-3", -3));
  EXPECT_TRUE(EvaluateAndCheck("-123456", -123456));
  EXPECT_TRUE(
      EvaluateAndCheck(std::to_string(-std::numeric_limits<int64_t>::max()),
                       -std::numeric_limits<int64_t>::max()));
  EXPECT_THAT(ParseExpression("-12345678901234567890"), IsOverflow());

  // TODO(b/208295758): INT_MIN does not work
}

TEST(ExpressionsTest, AdditionWorks) {
  EXPECT_TRUE(EvaluateAndCheck("0 + 0", 0 + 0));    // 0, 0
  EXPECT_TRUE(EvaluateAndCheck("1 + 0", 1 + 0));    // Pos, 0
  EXPECT_TRUE(EvaluateAndCheck("0 + 2", 0 + 2));    // 0, Pos
  EXPECT_TRUE(EvaluateAndCheck("-1 + 0", -1 + 0));  // Neg, 0
  EXPECT_TRUE(EvaluateAndCheck("0 + -2", 0 + -2));  // 0, Neg

  EXPECT_TRUE(EvaluateAndCheck("5 + 5", 5 + 5));  // PosEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("3 + 2", 3 + 2));  // PosLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("2 + 4", 2 + 4));  // PosSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("-5 + 5", -5 + 5));  // NegEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("-3 + 2", -3 + 2));  // NegLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("-2 + 4", -2 + 4));  // NegSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("5 + -5", 5 + -5));  // PosEqual, NegEqual
  EXPECT_TRUE(EvaluateAndCheck("3 + -2", 3 + -2));  // PosLarge, NegSmall
  EXPECT_TRUE(EvaluateAndCheck("2 + -4", 2 + -4));  // PosSmall, NegLarge

  EXPECT_TRUE(EvaluateAndCheck("-5 + -5", -5 + -5));  // NegEqual, NegEqual
  EXPECT_TRUE(EvaluateAndCheck("-3 + -2", -3 + -2));  // NegLarge, NegSmall
  EXPECT_TRUE(EvaluateAndCheck("-2 + -4", -2 + -4));  // NegSmall, NegLarge
}

TEST(ExpressionsTest, SubtractionWorks) {
  EXPECT_TRUE(EvaluateAndCheck("0 - 0", 0 - 0));    // 0, 0
  EXPECT_TRUE(EvaluateAndCheck("1 - 0", 1 - 0));    // Pos, 0
  EXPECT_TRUE(EvaluateAndCheck("0 - 2", 0 - 2));    // 0, Pos
  EXPECT_TRUE(EvaluateAndCheck("-1 - 0", -1 - 0));  // Neg, 0
  EXPECT_TRUE(EvaluateAndCheck("0 - -2", 0 - -2));  // 0, Neg

  EXPECT_TRUE(EvaluateAndCheck("5 - 5", 5 - 5));  // PosEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("3 - 2", 3 - 2));  // PosLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("2 - 4", 2 - 4));  // PosSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("-5 - 5", -5 - 5));  // NegEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("-3 - 2", -3 - 2));  // NegLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("-2 - 4", -2 - 4));  // NegSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("5 - -5", 5 - -5));  // PosEqual, NegEqual
  EXPECT_TRUE(EvaluateAndCheck("3 - -2", 3 - -2));  // PosLarge, NegSmall
  EXPECT_TRUE(EvaluateAndCheck("2 - -4", 2 - -4));  // PosSmall, NegLarge

  EXPECT_TRUE(EvaluateAndCheck("-5 - -5", -5 - -5));  // NegEqual, NegEqual
  EXPECT_TRUE(EvaluateAndCheck("-3 - -2", -3 - -2));  // NegLarge, NegSmall
  EXPECT_TRUE(EvaluateAndCheck("-2 - -4", -2 - -4));  // NegSmall, NegLarge
}

TEST(ExpressionsTest, MultiplicationWorks) {
  EXPECT_TRUE(EvaluateAndCheck("0 * 0", 0 * 0));    // 0, 0
  EXPECT_TRUE(EvaluateAndCheck("1 * 0", 1 * 0));    // Pos, 0
  EXPECT_TRUE(EvaluateAndCheck("0 * 2", 0 * 2));    // 0, Pos
  EXPECT_TRUE(EvaluateAndCheck("-1 * 0", -1 * 0));  // Neg, 0
  EXPECT_TRUE(EvaluateAndCheck("0 * -2", 0 * -2));  // 0, Neg

  EXPECT_TRUE(EvaluateAndCheck("5 * 5", 5 * 5));  // PosEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("3 * 2", 3 * 2));  // PosLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("2 * 4", 2 * 4));  // PosSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("-5 * 5", -5 * 5));  // NegEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("-3 * 2", -3 * 2));  // NegLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("-2 * 4", -2 * 4));  // NegSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("5 * -5", 5 * -5));  // PosEqual, NegEqual
  EXPECT_TRUE(EvaluateAndCheck("3 * -2", 3 * -2));  // PosLarge, NegSmall
  EXPECT_TRUE(EvaluateAndCheck("2 * -4", 2 * -4));  // PosSmall, NegLarge

  EXPECT_TRUE(EvaluateAndCheck("-5 * -5", -5 * -5));  // NegEqual, NegEqual
  EXPECT_TRUE(EvaluateAndCheck("-3 * -2", -3 * -2));  // NegLarge, NegSmall
  EXPECT_TRUE(EvaluateAndCheck("-2 * -4", -2 * -4));  // NegSmall, NegLarge
}

TEST(ExpressionsTest, DivisionWorks) {
  EXPECT_TRUE(EvaluateAndCheck("0 / 2", 0 / 2));    // 0, Pos
  EXPECT_TRUE(EvaluateAndCheck("0 / -2", 0 / -2));  // 0, Neg

  EXPECT_TRUE(EvaluateAndCheck("5 / 5", 5 / 5));  // PosEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("3 / 2", 3 / 2));  // PosLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("2 / 4", 2 / 4));  // PosSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("-5 / 5", -5 / 5));  // NegEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("-3 / 2", -3 / 2));  // NegLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("-2 / 4", -2 / 4));  // NegSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("5 / -5", 5 / -5));  // PosEqual, NegEqual
  EXPECT_TRUE(EvaluateAndCheck("3 / -2", 3 / -2));  // PosLarge, NegSmall
  EXPECT_TRUE(EvaluateAndCheck("2 / -4", 2 / -4));  // PosSmall, NegLarge

  EXPECT_TRUE(EvaluateAndCheck("-5 / -5", -5 / -5));  // NegEqual, NegEqual
  EXPECT_TRUE(EvaluateAndCheck("-3 / -2", -3 / -2));  // NegLarge, NegSmall
  EXPECT_TRUE(EvaluateAndCheck("-2 / -4", -2 / -4));  // NegSmall, NegLarge
}

TEST(ExpressionsTest, ModulusWorks) {
  EXPECT_TRUE(EvaluateAndCheck("0 % 2", 0 % 2));    // 0, Pos
  EXPECT_TRUE(EvaluateAndCheck("0 % -2", 0 % -2));  // 0, Neg

  EXPECT_TRUE(EvaluateAndCheck("5 % 5", 5 % 5));  // PosEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("3 % 2", 3 % 2));  // PosLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("2 % 4", 2 % 4));  // PosSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("-5 % 5", -5 % 5));  // NegEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("-3 % 2", -3 % 2));  // NegLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("-2 % 4", -2 % 4));  // NegSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("5 % -5", 5 % -5));  // PosEqual, NegEqual
  EXPECT_TRUE(EvaluateAndCheck("3 % -2", 3 % -2));  // PosLarge, NegSmall
  EXPECT_TRUE(EvaluateAndCheck("2 % -4", 2 % -4));  // PosSmall, NegLarge

  EXPECT_TRUE(EvaluateAndCheck("-5 % -5", -5 % -5));  // NegEqual, NegEqual
  EXPECT_TRUE(EvaluateAndCheck("-3 % -2", -3 % -2));  // NegLarge, NegSmall
  EXPECT_TRUE(EvaluateAndCheck("-2 % -4", -2 % -4));  // NegSmall, NegLarge
}

// Basic exponentiation in O(exp)
int64_t pow(int64_t base, int64_t exp) {
  int64_t val = 1;
  while (exp--) val *= base;
  return val;
}

TEST(ExpressionsTest, ExponentiationWorks) {
  EXPECT_TRUE(EvaluateAndCheck("1 ^ 0", pow(1, 0)));      // Pos, 0
  EXPECT_TRUE(EvaluateAndCheck("0 ^ 2", pow(0, 2)));      // 0, Pos
  EXPECT_TRUE(EvaluateAndCheck("-1 ^ 0", -pow(1, 0)));    // Neg, 0
  EXPECT_TRUE(EvaluateAndCheck("(-1) ^ 0", pow(-1, 0)));  // Neg, 0

  // Bracketted vs not. -1 ^ 2 == -(1 ^ 2) to maintain parity with Python.
  EXPECT_TRUE(EvaluateAndCheck("(-1) ^ 2", pow(-1, 2)));  // Neg, 0
  EXPECT_TRUE(EvaluateAndCheck("-1 ^ 2", -pow(1, 2)));    // Neg, 0

  EXPECT_TRUE(EvaluateAndCheck("5 ^ 5", pow(5, 5)));  // PosEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("3 ^ 2", pow(3, 2)));  // PosLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("2 ^ 4", pow(2, 4)));  // PosSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("-5 ^ 5", -pow(5, 5)));  // NegEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("-3 ^ 2", -pow(3, 2)));  // NegLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("-2 ^ 4", -pow(2, 4)));  // NegSmall, PosLarge

  EXPECT_TRUE(EvaluateAndCheck("(-5) ^ 5", pow(-5, 5)));  // NegEqual, PosEqual
  EXPECT_TRUE(EvaluateAndCheck("(-3) ^ 2", pow(-3, 2)));  // NegLarge, PosSmall
  EXPECT_TRUE(EvaluateAndCheck("(-2) ^ 4", pow(-2, 4)));  // NegSmall, PosLarge

  // Huge exponentiation for 0, 1, -1 is okay and fast.
  EXPECT_TRUE(EvaluateAndCheck("0 ^ 123456789012345678", 0));
  EXPECT_TRUE(EvaluateAndCheck("1 ^ 123456789012345678", 1));
  EXPECT_TRUE(EvaluateAndCheck("(-1) ^ 123456789012345678", 1));
  EXPECT_TRUE(EvaluateAndCheck("(-1) ^ 123456789012345677", -1));
}

TEST(ExpressionsTest, OrderOfOperationsAndAssociativityBehaves) {
  EXPECT_TRUE(EvaluateAndCheck("3 + 7 + 5", 3 + 7 + 5));
  EXPECT_TRUE(EvaluateAndCheck("3 + 7 - 5", 3 + 7 - 5));
  EXPECT_TRUE(EvaluateAndCheck("3 + 7 * 5", 3 + 7 * 5));
  EXPECT_TRUE(EvaluateAndCheck("3 + 7 / 5", 3 + 7 / 5));
  EXPECT_TRUE(EvaluateAndCheck("3 + 7 % 5", 3 + 7 % 5));
  EXPECT_TRUE(EvaluateAndCheck("3 + 7 ^ 5", 3 + pow(7, 5)));

  EXPECT_TRUE(EvaluateAndCheck("3 - 7 + 5", 3 - 7 + 5));
  EXPECT_TRUE(EvaluateAndCheck("3 - 7 - 5", 3 - 7 - 5));
  EXPECT_TRUE(EvaluateAndCheck("3 - 7 * 5", 3 - 7 * 5));
  EXPECT_TRUE(EvaluateAndCheck("3 - 7 / 5", 3 - 7 / 5));
  EXPECT_TRUE(EvaluateAndCheck("3 - 7 % 5", 3 - 7 % 5));
  EXPECT_TRUE(EvaluateAndCheck("3 - 7 ^ 5", 3 - pow(7, 5)));

  EXPECT_TRUE(EvaluateAndCheck("3 * 7 + 5", 3 * 7 + 5));
  EXPECT_TRUE(EvaluateAndCheck("3 * 7 - 5", 3 * 7 - 5));
  EXPECT_TRUE(EvaluateAndCheck("3 * 7 * 5", 3 * 7 * 5));
  EXPECT_TRUE(EvaluateAndCheck("3 * 7 / 5", 3 * 7 / 5));
  EXPECT_TRUE(EvaluateAndCheck("3 * 7 % 5", 3 * 7 % 5));
  EXPECT_TRUE(EvaluateAndCheck("3 * 7 ^ 5", 3 * pow(7, 5)));

  EXPECT_TRUE(EvaluateAndCheck("123456789 / 7 + 5", 123456789 / 7 + 5));
  EXPECT_TRUE(EvaluateAndCheck("123456789 / 7 - 5", 123456789 / 7 - 5));
  EXPECT_TRUE(EvaluateAndCheck("123456789 / 7 * 5", 123456789 / 7 * 5));
  EXPECT_TRUE(EvaluateAndCheck("123456789 / 7 / 5", 123456789 / 7 / 5));
  EXPECT_TRUE(EvaluateAndCheck("123456789 / 7 % 5", 123456789 / 7 % 5));
  EXPECT_TRUE(EvaluateAndCheck("123456789 / 7 ^ 5", 123456789 / pow(7, 5)));

  EXPECT_TRUE(EvaluateAndCheck("123456789 % 7 + 5", 123456789 % 7 + 5));
  EXPECT_TRUE(EvaluateAndCheck("123456789 % 7 - 5", 123456789 % 7 - 5));
  EXPECT_TRUE(EvaluateAndCheck("123456789 % 7 * 5", 123456789 % 7 * 5));
  EXPECT_TRUE(EvaluateAndCheck("123456789 % 7 / 5", 123456789 % 7 / 5));
  EXPECT_TRUE(EvaluateAndCheck("123456789 % 7 % 5", 123456789 % 7 % 5));
  EXPECT_TRUE(EvaluateAndCheck("123456789 % 7 ^ 5", 123456789 % pow(7, 5)));

  EXPECT_TRUE(EvaluateAndCheck("3 ^ 7 + 5", pow(3, 7) + 5));
  EXPECT_TRUE(EvaluateAndCheck("3 ^ 7 - 5", pow(3, 7) - 5));
  EXPECT_TRUE(EvaluateAndCheck("3 ^ 7 * 5", pow(3, 7) * 5));
  EXPECT_TRUE(EvaluateAndCheck("3 ^ 7 / 5", pow(3, 7) / 5));
  EXPECT_TRUE(EvaluateAndCheck("3 ^ 7 % 5", pow(3, 7) % 5));
  EXPECT_TRUE(EvaluateAndCheck("4 ^ 3 ^ 2", pow(4, pow(3, 2))));
}

TEST(ExpressionsTest, ParenthesesOverrideOrderOfOperations) {
  EXPECT_TRUE(EvaluateAndCheck("3 + (7 + 5)", 3 + (7 + 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 + (7 - 5)", 3 + (7 - 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 + (7 * 5)", 3 + (7 * 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 + (7 / 5)", 3 + (7 / 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 + (7 % 5)", 3 + (7 % 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 + (7 ^ 5)", 3 + pow(7, 5)));

  EXPECT_TRUE(EvaluateAndCheck("3 - (7 + 5)", 3 - (7 + 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 - (7 - 5)", 3 - (7 - 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 - (7 * 5)", 3 - (7 * 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 - (7 / 5)", 3 - (7 / 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 - (7 % 5)", 3 - (7 % 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 - (7 ^ 5)", 3 - pow(7, 5)));

  EXPECT_TRUE(EvaluateAndCheck("3 * (7 + 5)", 3 * (7 + 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 * (7 - 5)", 3 * (7 - 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 * (7 * 5)", 3 * (7 * 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 * (7 / 5)", 3 * (7 / 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 * (7 % 5)", 3 * (7 % 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 * (7 ^ 5)", 3 * pow(7, 5)));

  EXPECT_TRUE(EvaluateAndCheck("123456789 / (7 + 5)", 123456789 / (7 + 5)));
  EXPECT_TRUE(EvaluateAndCheck("123456789 / (7 - 5)", 123456789 / (7 - 5)));
  EXPECT_TRUE(EvaluateAndCheck("123456789 / (7 * 5)", 123456789 / (7 * 5)));
  EXPECT_TRUE(EvaluateAndCheck("123456789 / (7 / 5)", 123456789 / (7 / 5)));
  EXPECT_TRUE(EvaluateAndCheck("123456789 / (7 % 5)", 123456789 / (7 % 5)));
  EXPECT_TRUE(EvaluateAndCheck("123456789 / (7 ^ 5)", 123456789 / pow(7, 5)));

  EXPECT_TRUE(EvaluateAndCheck("123456789 % (7 + 5)", 123456789 % (7 + 5)));
  EXPECT_TRUE(EvaluateAndCheck("123456789 % (7 - 5)", 123456789 % (7 - 5)));
  EXPECT_TRUE(EvaluateAndCheck("123456789 % (7 * 5)", 123456789 % (7 * 5)));
  EXPECT_TRUE(EvaluateAndCheck("123456789 % (7 / 5)", 123456789 % (7 / 5)));
  EXPECT_TRUE(EvaluateAndCheck("123456789 % (7 % 5)", 123456789 % (7 % 5)));
  EXPECT_TRUE(EvaluateAndCheck("123456789 % (7 ^ 5)", 123456789 % pow(7, 5)));

  EXPECT_TRUE(EvaluateAndCheck("3 ^ (7 + 5)", pow(3, 7 + 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 ^ (7 - 5)", pow(3, 7 - 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 ^ (7 * 5)", pow(3, 7 * 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 ^ (7 / 5)", pow(3, 7 / 5)));
  EXPECT_TRUE(EvaluateAndCheck("3 ^ (7 % 5)", pow(3, 7 % 5)));
  EXPECT_TRUE(EvaluateAndCheck("(4 ^ 3) ^ 2", pow(pow(4, 3), 2)));
}

TEST(ExpressionsTest, NestedAndSideBySideParenthesesWork) {
  EXPECT_TRUE(EvaluateAndCheck("(2 + 3) * (4 + 5)", (2 + 3) * (4 + 5)));
  EXPECT_TRUE(EvaluateAndCheck("((2 + 3) * (4 + 5))", (2 + 3) * (4 + 5)));
  EXPECT_TRUE(EvaluateAndCheck("((((((2 + 3))))))", 2 + 3));

  EXPECT_TRUE(EvaluateAndCheck("(+2 + 3) * (4 + -5)", (+2 + 3) * (4 + -5)));
  EXPECT_TRUE(EvaluateAndCheck("((+2 + 3) * (-4 + 5))", (+2 + 3) * (-4 + 5)));
  EXPECT_TRUE(EvaluateAndCheck("((((((+2 + +3))))))", +2 + +3));
}

// TODO(b/208295758): Should "()" be acceptable? "() 1" currently evaluates to 1
TEST(ExpressionsTest, ImproperNesting) {
  EXPECT_THAT(ParseExpression("(1"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("open parenthesis without")));

  EXPECT_THAT(ParseExpression("((1)"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("open parenthesis without")));

  EXPECT_THAT(ParseExpression("1)"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Unmatched closed parenthesis")));

  EXPECT_THAT(ParseExpression("(1))"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Unmatched closed parenthesis")));
}

TEST(ExpressionsTest, BackToBackNumbers) {
  EXPECT_THAT(ParseExpression("1 1"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ExpressionsTest, BinaryOperatorsWithoutTwoArguments) {
  EXPECT_THAT(ParseExpression("1 *"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("1 * ) * 1"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("* 3 + 4"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ExpressionsTest, UnaryOperatorsWithoutAnArgument) {
  EXPECT_THAT(ParseExpression("-"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("4 +  (-)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ExpressionsTest, UnaryPlusAndMinusAreIdentified) {
  EXPECT_TRUE(EvaluateAndCheck("3  + (-10 + 1)", 3 + (-10 + 1)));
  EXPECT_TRUE(EvaluateAndCheck("-(4 * -4)", -(4 * -4)));
  EXPECT_THAT(ParseExpression("--42"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("-+42"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ExpressionsTest, WhitespaceShouldBeIgnored) {
  EXPECT_TRUE(EvaluateAndCheck(" 3 + 1", 3 + 1));
  EXPECT_TRUE(EvaluateAndCheck("3 + 1 ", 3 + 1));
  EXPECT_TRUE(EvaluateAndCheck(" 3 + 1 ", 3 + 1));
  EXPECT_TRUE(EvaluateAndCheck(" (  3   + - 1 - 1 ) * 4 ", (3 + -1 - 1) * 4));
}

TEST(ExpressionsTest, InvalidCharacter) {
  EXPECT_THAT(ParseExpression("~"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("3 + &"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

/* -------------------------------------------------------------------------- */
/*  STRING PARSING WITH VARIABLES                                             */
/* -------------------------------------------------------------------------- */

[[nodiscard]] testing::AssertionResult EvaluateAndCheck(
    absl::string_view expression,
    const absl::flat_hash_map<std::string, int64_t>& variables,
    int64_t expected) {
  absl::StatusOr<Expression> exp = ParseExpression(expression);
  if (!exp.ok())
    return testing::AssertionFailure()
           << std::quoted(expression) << " failed to parse; " << exp.status();

  absl::StatusOr<int64_t> val = EvaluateIntegerExpression(*exp, variables);
  if (!val.ok())
    return testing::AssertionFailure()
           << std::quoted(expression)
           << " failed to evaluate after successfully parsing; "
           << val.status();

  if (*val != expected)
    return testing::AssertionFailure()
           << std::quoted(expression) << " evaluated to " << *val;
  return testing::AssertionSuccess();
}

absl::StatusOr<int64_t> Evaluate(
    absl::string_view expression,
    const absl::flat_hash_map<std::string, int64_t>& variables = {}) {
  MORIARTY_ASSIGN_OR_RETURN(Expression expr, ParseExpression(expression));

  return EvaluateIntegerExpression(expr, variables);
}

TEST(ExpressionsTest, SingleVariableParsesProperly) {
  EXPECT_TRUE(EvaluateAndCheck("N", {{"N", 2}}, 2));
  EXPECT_TRUE(
      EvaluateAndCheck("multiple_letters_4", {{"multiple_letters_4", 2}}, 2));
  EXPECT_TRUE(EvaluateAndCheck("3 * N + 1", {{"N", 2}}, 7));
  EXPECT_TRUE(EvaluateAndCheck("1 + 3 * N", {{"N", 2}}, 7));
  EXPECT_TRUE(EvaluateAndCheck("N ^ N", {{"N", 4}}, pow(4, 4)));
}

TEST(ExpressionsTest, MultipleVariableParsesProperly) {
  EXPECT_TRUE(EvaluateAndCheck("X + Y", {{"X", 2}, {"Y", 4}}, 6));
  EXPECT_TRUE(EvaluateAndCheck("X * Y + 1", {{"X", 2}, {"Y", 4}}, 9));
  EXPECT_TRUE(EvaluateAndCheck("Y + Y / X", {{"X", 2}, {"Y", 4}}, 6));
  EXPECT_TRUE(EvaluateAndCheck("X ^ Y", {{"X", 3}, {"Y", 4}}, pow(3, 4)));
  EXPECT_TRUE(EvaluateAndCheck("X + X + Y * Y + X * Y", {{"X", 3}, {"Y", 4}},
                               3 + 3 + 4 * 4 + 3 * 4));
}

TEST(ExpressionsTest, MissingVariablesFails) {
  EXPECT_THAT(Evaluate("X", {}), StatusIs(absl::StatusCode::kInvalidArgument,
                                          HasSubstr("unknown value: X")));
  EXPECT_THAT(Evaluate("X + -Y", {}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("unknown value: X")));
  EXPECT_THAT(Evaluate("X + Y", {{"Y", 33}}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("unknown value: X")));
  EXPECT_THAT(Evaluate("-X + X + Y", {{"Y", 33}}),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("unknown value: X")));
}

TEST(ExpressionsTest, NeededVariables) {
  auto needed_vars = [](absl::string_view expression)
      -> absl::StatusOr<absl::flat_hash_set<std::string>> {
    MORIARTY_ASSIGN_OR_RETURN(Expression expr, ParseExpression(expression));
    return NeededVariables(expr);
  };

  EXPECT_THAT(needed_vars("3 + 1"), IsOkAndHolds(UnorderedElementsAre()));
  EXPECT_THAT(needed_vars("3 * N + 1"),
              IsOkAndHolds(UnorderedElementsAre("N")));
  EXPECT_THAT(needed_vars("X * Y + 5"),
              IsOkAndHolds(UnorderedElementsAre("X", "Y")));
  EXPECT_THAT(needed_vars("multiple_letters_4 * N + 1"),
              IsOkAndHolds(UnorderedElementsAre("multiple_letters_4", "N")));
  EXPECT_THAT(needed_vars("multiple_letters_4 * N + 1"),
              IsOkAndHolds(UnorderedElementsAre("multiple_letters_4", "N")));
  EXPECT_THAT(needed_vars("X * Y + X"),
              IsOkAndHolds(UnorderedElementsAre("X", "Y")));
}

/* -------------------------------------------------------------------------- */
/*  STRING PARSING WITH FUNCTIONS                                             */
/* -------------------------------------------------------------------------- */

TEST(ExpressionsTest, FunctionsWork) {
  EXPECT_TRUE(EvaluateAndCheck("min(3, 5)", 3));
  EXPECT_TRUE(EvaluateAndCheck("min(1, 2, -3, 0)", -3));
  EXPECT_TRUE(EvaluateAndCheck("min(max(5, 20, 10), max(10, 11, 12))", 12));
  EXPECT_TRUE(EvaluateAndCheck("min(3)", 3));
  EXPECT_TRUE(EvaluateAndCheck("abs(-3)", 3));
  EXPECT_TRUE(
      EvaluateAndCheck("min(max(5, abs(-20), 10), max(10, 11, abs(-12)))", 12));
  EXPECT_TRUE(EvaluateAndCheck("min(abs(-5), 10)", 5));
}

TEST(ExpressionsTest, InvalidFunctionArgumentsShouldFail) {
  EXPECT_THAT(Evaluate("(1, 2, 3)"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("Incomplete function")));  // No name
  EXPECT_THAT(Evaluate("f()"), StatusIs(absl::StatusCode::kInvalidArgument,
                                        HasSubstr("()")));  // Unknown name

  EXPECT_THAT(Evaluate("abs()"),
              StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("()")));
  EXPECT_THAT(
      Evaluate("abs(3, 4)"),
      StatusIs(absl::StatusCode::kInvalidArgument, HasSubstr("one parameter")));
}

// TODO(darcybest): With the current implementation, these pass, but shouldn't:
//   f((1, 2))    ==> Interpreted as f(1, 2)
//   f(1, (2, 3)) ==> Interpreted as f(1, 2, 3)
TEST(ExpressionsTest, InvalidCommasInArgumentShouldFail) {
  EXPECT_THAT(ParseExpression("f(1, , 2)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("f("),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("f( , 1)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("f(1, )"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("(1, 2)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("1 + (1, 2)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("(1, 2) + 3"),
              StatusIs(absl::StatusCode::kInvalidArgument));

  // This may see that there are 3 arguments to g, then assume they are "g",
  // "x", "y", then pop "f" off the stack instead of "g".
  EXPECT_THAT(ParseExpression("f(, g(x,y,)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_THAT(ParseExpression("1(g(,2,3)"),
              StatusIs(absl::StatusCode::kInvalidArgument));
}

TEST(ExpressionsTest, FunctionsAndVariablesMixWell) {
  EXPECT_TRUE(EvaluateAndCheck("min(3, N)", {{"N", 2}}, 2));
  EXPECT_TRUE(EvaluateAndCheck("min(3, N)", {{"N", 3}}, 3));
  EXPECT_TRUE(EvaluateAndCheck("min(3, N)", {{"N", 4}}, 3));

  EXPECT_TRUE(EvaluateAndCheck("min(M, N)", {{"M", 3}, {"N", 2}}, 2));
}

TEST(ExpressionsTest, FunctionsAndVariablesMayHaveTheSameName) {
  EXPECT_TRUE(EvaluateAndCheck("min(min, 3)", {{"min", 2}}, 2));
  EXPECT_TRUE(EvaluateAndCheck("min(min, max)", {{"min", 3}, {"max", 5}}, 3));
}

/* -------------------------------------------------------------------------- */
/*  EXPRESSION CLASSES                                                        */
/* -------------------------------------------------------------------------- */

}  // namespace

bool operator==(const Expression& lhs, const Expression& rhs);

namespace moriarty_internal {

bool operator==(const Literal& lhs, const Literal& rhs) {
  if (lhs.IsVariable() != rhs.IsVariable()) return false;
  return lhs.IsVariable() ? lhs.IsVariable() == rhs.IsVariable()
                          : lhs.Value() == rhs.Value();
}

bool operator==(const BinaryOperation& lhs, const BinaryOperation& rhs) {
  return lhs.Op() == rhs.Op() && lhs.Lhs() == rhs.Lhs() &&
         lhs.Rhs() == rhs.Rhs();
}

bool operator==(const UnaryOperation& lhs, const UnaryOperation& rhs) {
  return lhs.Op() == rhs.Op() && lhs.Rhs() == rhs.Rhs();
}

bool operator==(const Function& lhs, const Function& rhs) {
  if (lhs.Name() != rhs.Name()) return false;
  if (lhs.Arguments().size() != rhs.Arguments().size()) return false;
  for (int i = 0; i < lhs.Arguments().size(); i++) {
    if (lhs.Arguments()[i] != rhs.Arguments()[i]) return false;
  }
  return true;
}

}  // namespace moriarty_internal

bool operator==(const Expression& lhs, const Expression& rhs) {
  return lhs.Get() == rhs.Get();
}

namespace {

TEST(ExpressionsTest, BinaryOperationsAccessorsWork) {
  UnaryOperation rhs = UnaryOperation(UnaryOperator::kPlus, Literal(456));
  BinaryOperation expr(Literal(123), BinaryOperator::kAdd, rhs);

  EXPECT_THAT(expr.Lhs(), Literal(123));
  EXPECT_EQ(expr.Op(), BinaryOperator::kAdd);
  EXPECT_THAT(expr.Rhs(), rhs);
}

TEST(ExpressionsTest, BinaryOperationsCopyAccessorsWork) {
  UnaryOperation rhs = UnaryOperation(UnaryOperator::kPlus, Literal(456));
  BinaryOperation expr(Literal(123), BinaryOperator::kAdd, rhs);

  BinaryOperation expr2 = expr;
  expr = BinaryOperation(Literal(456), BinaryOperator::kSubtract, Literal(789));

  EXPECT_EQ(expr2.Lhs(), Literal(123));
  EXPECT_EQ(expr2.Op(), BinaryOperator::kAdd);
  EXPECT_EQ(expr2.Rhs(), rhs);
}

TEST(ExpressionsTest, UnaryOperationsAccessorsWork) {
  UnaryOperation expr(UnaryOperator::kPlus, Literal(123));

  EXPECT_EQ(expr.Op(), UnaryOperator::kPlus);
  EXPECT_EQ(expr.Rhs(), Literal(123));
}

TEST(ExpressionsTest, UnaryOperationsCopyAccessorsWork) {
  UnaryOperation expr(UnaryOperator::kPlus, Literal(123));

  UnaryOperation expr2 = expr;
  expr = UnaryOperation(UnaryOperator::kNegate, Literal(456));

  // Copied expr over
  EXPECT_EQ(expr2.Op(), UnaryOperator::kPlus);
  EXPECT_EQ(expr2.Rhs(), Literal(123));
}

TEST(ExpressionsTest, FunctionAccessorsWork) {
  std::vector<std::unique_ptr<Expression>> args;
  args.push_back(std::make_unique<Expression>(Literal(123)));
  args.push_back(std::make_unique<Expression>(Literal(456)));
  Function fn("name", std::move(args));

  EXPECT_EQ(fn.Name(), "name");
  EXPECT_THAT(fn.Arguments(),
              ElementsAre(Pointee(Literal(123)), Pointee(Literal(456))));
}

TEST(ExpressionsTest, FunctionCopyAccessorsWork) {
  std::vector<std::unique_ptr<Expression>> args;
  args.push_back(std::make_unique<Expression>(Literal(110)));
  args.push_back(std::make_unique<Expression>(Literal(111)));

  std::vector<std::unique_ptr<Expression>> args2;
  args2.push_back(std::make_unique<Expression>(Literal(220)));
  args2.push_back(std::make_unique<Expression>(Literal(221)));

  Function fn("name", std::move(args));
  Function fn2 = fn;
  fn = Function("name2", std::move(args2));

  EXPECT_EQ(fn2.Name(), "name");
  EXPECT_THAT(fn2.Arguments(),
              ElementsAre(Pointee(Literal(110)), Pointee(Literal(111))));
}

TEST(ExpressionsTest, FunctionUpdateNameWorks) {
  Function fn("name", {});

  fn.SetName("name2");

  EXPECT_EQ(fn.Name(), "name2");
}

TEST(ExpressionsTest, FunctionReverseArgumentsWork) {
  // 0 args, even number of arguments and odd number of arguments.
  std::vector<std::unique_ptr<Expression>> args_zero;
  Function fn_zero("name", std::move(args_zero));

  std::vector<std::unique_ptr<Expression>> args_even;
  args_even.push_back(std::make_unique<Expression>(Literal(111)));
  args_even.push_back(std::make_unique<Expression>(Literal(222)));
  Function fn_even("name", std::move(args_even));

  std::vector<std::unique_ptr<Expression>> args_odd;
  args_odd.push_back(std::make_unique<Expression>(Literal(444)));
  args_odd.push_back(std::make_unique<Expression>(Literal(555)));
  args_odd.push_back(std::make_unique<Expression>(Literal(666)));
  Function fn_odd("name", std::move(args_odd));

  fn_zero.ReverseArguments();
  fn_even.ReverseArguments();
  fn_odd.ReverseArguments();

  EXPECT_THAT(fn_zero.Arguments(), IsEmpty());
  EXPECT_THAT(fn_even.Arguments(),
              ElementsAre(Pointee(Literal(222)), Pointee(Literal(111))));
  EXPECT_THAT(fn_odd.Arguments(),
              ElementsAre(Pointee(Literal(666)), Pointee(Literal(555)),
                          Pointee(Literal(444))));
}

TEST(ExpressionsTest, FunctionAppendArgumentsWork) {
  std::vector<std::unique_ptr<Expression>> args;
  args.push_back(std::make_unique<Expression>(Literal(111)));
  Function fn("name", std::move(args));

  fn.AppendArgument(Literal(222));

  EXPECT_THAT(fn.Arguments(),
              ElementsAre(Pointee(Literal(111)), Pointee(Literal(222))));
}

TEST(ExpressionsTest, ExpressionAccessorsWork) {
  Literal lit(111);
  UnaryOperation unary(UnaryOperator::kPlus, Literal(222));
  BinaryOperation binary(Literal(333), BinaryOperator::kAdd, Literal(444));
  Function fn("name", {});

  EXPECT_THAT(Expression(lit).Get(), VariantWith<Literal>(lit));
  EXPECT_THAT(Expression(unary).Get(), VariantWith<UnaryOperation>(unary));
  EXPECT_THAT(Expression(binary).Get(), VariantWith<BinaryOperation>(binary));
  EXPECT_THAT(Expression(fn).Get(), VariantWith<Function>(fn));
}

TEST(ExpressionsTest, ExpressionCopyAccessorsWork) {
  Expression expr = UnaryOperation(UnaryOperator::kPlus, Literal(123));

  Expression expr2 = expr;
  expr = BinaryOperation(Literal(456), BinaryOperator::kAdd, Literal(789));

  // Copied expr over
  EXPECT_EQ(expr2, UnaryOperation(UnaryOperator::kPlus, Literal(123)));
}

TEST(ExpressionsTest, LiteralAccessorsWork) {
  Literal lit(123);
  Literal lit2("N");

  EXPECT_EQ(lit.Value(), 123);
  EXPECT_EQ(lit2.VariableName(), "N");
}

TEST(ExpressionsTest, LiteralCopyAccessorsWork) {
  Literal lit(123);
  Literal lit2 = lit;

  lit = Literal(456);

  EXPECT_EQ(lit2, Literal(123));
}

TEST(ExpressionsTest, IsVariableWorks) {
  Literal lit(123);
  Literal lit2("N");

  EXPECT_FALSE(lit.IsVariable());
  EXPECT_TRUE(lit2.IsVariable());
}

}  // namespace
}  // namespace moriarty
