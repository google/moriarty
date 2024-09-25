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

#include "src/variables/constraints/io_constraints.h"

#include "gtest/gtest.h"
#include "src/librarian/io_config.h"

namespace {

using ::moriarty::IOSeparator;
using ::moriarty::Whitespace;

TEST(IOSeparatorTest, GetSeparator) {
  EXPECT_EQ(IOSeparator(Whitespace::kSpace).GetSeparator(), Whitespace::kSpace);
  EXPECT_EQ(IOSeparator(Whitespace::kTab).GetSeparator(), Whitespace::kTab);
  EXPECT_EQ(IOSeparator(Whitespace::kNewline).GetSeparator(),
            Whitespace::kNewline);
}

TEST(IOSeparatorTest, ToString) {
  EXPECT_EQ(IOSeparator(Whitespace::kSpace).ToString(), "IOSeparator(Space)");
  EXPECT_EQ(IOSeparator(Whitespace::kTab).ToString(), "IOSeparator(Tab)");
  EXPECT_EQ(IOSeparator(Whitespace::kNewline).ToString(),
            "IOSeparator(Newline)");
}

}  // namespace
