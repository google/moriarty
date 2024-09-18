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

#include "src/variables/constraints/base_constraints.h"

#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>

#include "gtest/gtest.h"
#include "absl/strings/string_view.h"

namespace moriarty {
namespace {

TEST(BaseConstraintsTest, ExactlyForVariousIntegerTypesWorks) {
  static_assert(std::is_same_v<decltype(Exactly(123)), Exactly<int64_t>>);
  static_assert(std::is_same_v<decltype(Exactly(static_cast<int32_t>(123))),
                               Exactly<int64_t>>);
  static_assert(std::is_same_v<decltype(Exactly(static_cast<int64_t>(123))),
                               Exactly<int64_t>>);
  static_assert(std::is_same_v<decltype(Exactly(static_cast<uint32_t>(123))),
                               Exactly<int64_t>>);
  static_assert(std::is_same_v<decltype(Exactly(static_cast<uint64_t>(123))),
                               Exactly<int64_t>>);

  EXPECT_EQ(Exactly(123).GetValue(), int64_t{123});
  EXPECT_EQ(Exactly(static_cast<int32_t>(123)).GetValue(), int64_t{123});
  EXPECT_EQ(Exactly(static_cast<int64_t>(123)).GetValue(), int64_t{123});
  EXPECT_EQ(Exactly(static_cast<uint32_t>(123)).GetValue(), int64_t{123});
  EXPECT_EQ(Exactly(static_cast<uint64_t>(123)).GetValue(), int64_t{123});
}

TEST(BaseConstraintsTest, ExactlyForVariousStringLikeTypesWorks) {
  static_assert(std::is_same_v<decltype(Exactly("abc")), Exactly<std::string>>);
  static_assert(std::is_same_v<decltype(Exactly(std::string("abc"))),
                               Exactly<std::string>>);
  static_assert(std::is_same_v<decltype(Exactly(absl::string_view("abc"))),
                               Exactly<std::string>>);
  static_assert(std::is_same_v<decltype(Exactly(std::string_view("abc"))),
                               Exactly<std::string>>);

  EXPECT_EQ(Exactly("abc").GetValue(), std::string("abc"));
  EXPECT_EQ(Exactly(std::string("abc")).GetValue(), std::string("abc"));
  EXPECT_EQ(Exactly(absl::string_view("abc")).GetValue(), std::string("abc"));
  EXPECT_EQ(Exactly(std::string_view("abc")).GetValue(), std::string("abc"));
}

TEST(BaseConstraintsTest, ExactlyForOtherTypesWorks) {
  EXPECT_EQ(Exactly(std::set<int>({1, 2, 3})).GetValue(),
            std::set<int>({1, 2, 3}));
}

}  // namespace
}  // namespace moriarty
