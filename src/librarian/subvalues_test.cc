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

#include "src/librarian/subvalues.h"

#include <cstdint>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/status/status.h"
#include "src/internal/abstract_variable.h"
#include "src/util/test_status_macro/status_testutil.h"
#include "src/variables/minteger.h"
#include "src/variables/mstring.h"

namespace moriarty {
namespace librarian {
namespace {

using ::moriarty::IsOkAndHolds;
using ::moriarty::MInteger;
using ::moriarty::MString;
using ::moriarty::StatusIs;
using ::moriarty::moriarty_internal::SubvaluesManager;
using ::moriarty::moriarty_internal::VariableValue;
using ::testing::AnyWith;
using ::testing::Field;
using ::testing::Pointer;

TEST(SubvaluesTest, GetSubvalueDoesNotExistShouldFail) {
  Subvalues subvalues;
  EXPECT_THAT(SubvaluesManager(&subvalues).GetSubvalue("x"),
              StatusIs(absl::StatusCode::kNotFound));
}

TEST(SubvaluesTest, AddingASubvalueShouldWork) {
  Subvalues Subvalues;

  Subvalues.AddSubvalue<MInteger>("x", 10);

  EXPECT_THAT(SubvaluesManager(&Subvalues).GetSubvalue("x"),
              IsOkAndHolds(
                  Pointer(Field(&VariableValue::value, AnyWith<int64_t>(10)))));
}

TEST(SubvaluesTest, AddingMultipleSubvaluesShouldWork) {
  Subvalues Subvalues;
  MInteger x;
  MString y;

  Subvalues.AddSubvalue<MInteger>("x", 10);
  Subvalues.AddSubvalue<MString>("y", "hello");

  EXPECT_THAT(SubvaluesManager(&Subvalues).GetSubvalue("x"),
              IsOkAndHolds(
                  Pointer(Field(&VariableValue::value, AnyWith<int64_t>(10)))));
  EXPECT_THAT(SubvaluesManager(&Subvalues).GetSubvalue("y"),
              IsOkAndHolds(Pointer(Field(&VariableValue::value,
                                         AnyWith<std::string>("hello")))));
}

TEST(SubvaluesTest, AddingDuplicateSubvaluesShouldOverwriteTheValue) {
  Subvalues Subvalues;
  MInteger x;

  Subvalues.AddSubvalue<MInteger>("x", 10);
  Subvalues.AddSubvalue<MInteger>("x", 20);

  EXPECT_THAT(SubvaluesManager(&Subvalues).GetSubvalue("x"),
              IsOkAndHolds(
                  Pointer(Field(&VariableValue::value, AnyWith<int64_t>(20)))));
}

}  // namespace
}  // namespace librarian
}  // namespace moriarty
