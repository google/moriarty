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

#include "src/librarian/size_property.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/strings/string_view.h"
#include "src/internal/range.h"

namespace moriarty {
namespace librarian {

std::optional<CommonSize> MergeSizes(CommonSize size1, CommonSize size2) {
  if (size1 == size2) return size1;

  if (size1 == CommonSize::kAny) return size2;
  if (size2 == CommonSize::kAny) return size1;

  // All known relationships. Always take the subset.
  for (const auto& [subset, superset] :
       std::vector<std::pair<CommonSize, CommonSize>>{
           {CommonSize::kMin, CommonSize::kTiny},
           {CommonSize::kMin, CommonSize::kSmall},
           {CommonSize::kTiny, CommonSize::kSmall},
           {CommonSize::kMax, CommonSize::kHuge},
           {CommonSize::kMax, CommonSize::kLarge},
           {CommonSize::kHuge, CommonSize::kLarge}}) {
    if ((size1 == subset && size2 == superset) ||
        (size2 == subset && size1 == superset))
      return subset;
  }
  return std::nullopt;
}

moriarty::Range GetRange(CommonSize size, int64_t N) {
  switch (size) {
    case CommonSize::kAny:
      return moriarty::Range(1, N);
    case CommonSize::kMin:
      return GetMinRange(N);
    case CommonSize::kMax:
      return GetMaxRange(N);
    case CommonSize::kSmall:
      return GetSmallRange(N);
    case CommonSize::kMedium:
      return GetMediumRange(N);
    case CommonSize::kLarge:
      return GetLargeRange(N);
    case CommonSize::kTiny:
      return GetTinyRange(N);
    case CommonSize::kHuge:
      return GetHugeRange(N);
    default:
      ABSL_CHECK(false) << "Unknown size";
  }
}

namespace {

int64_t GetSmallMaximumThreshold(int64_t N) {
  // For very small N, this is a close enough approximation.
  if (N <= 100) return std::sqrt(N);

  // For larger values of N, the cutoff is selected (somewhat arbitrarily) to
  // correspond to common competitive programming contest cutoffs. For
  // example, if N <= 1000, then it is common for the problem to expect O(N^2)
  // solutions. For "small" N, we will allow O(N^4) solutions to pass.
  if (N <= 300) return 30;       // O(N^3) --> allow O(N^5)
  if (N <= 5000) return 100;     // O(N^2) --> allow O(N^4)
  if (N <= 5000000) return 300;  // O(N)    --> allow O(N^3)
  return 2000;                   // Everything else, allow O(N^2)
}

int64_t GetMediumMaximumThreshold(int64_t N) {
  // For very small N, this is a close enough approximation.
  if (N <= 100)
    return std::clamp<int64_t>(N / 2, GetSmallMaximumThreshold(N), N);

  // For larger values of N, the cutoff is selected (somewhat arbitrarily) to
  // correspond to common competitive programming contest cutoffs. For
  // example, if N <= 1000, then it is common for the problem to expect O(N^2)
  // solutions. For "small" N, we will allow O(N^4) solutions to pass.
  if (N <= 300) return 100;       // O(N^3) --> allow O(N^4)
  if (N <= 5000) return 500;      // O(N^2) --> allow O(N^3)
  if (N <= 5000000) return 5000;  // O(N)   --> allow O(N^2)
  return 1000000;                 // Everything else, allow O(N)
}

int64_t GetTinyMaximumThreshold(int64_t N) {
  int64_t small = GetSmallMaximumThreshold(N);
  if (small <= 10) return small;
  return std::max<int64_t>(10, std::log2(N));
}

int64_t GetHugeMinimumThreshold(int64_t N) {
  if (N <= 10) return GetMediumMaximumThreshold(N);
  return (N / 10) * 9;  // Note: N / 10 >= 1 now.
}

}  // namespace

moriarty::Range GetMinRange(int64_t N) {
  if (N <= 0) return moriarty::EmptyRange();
  return Range(1, 1);
}

moriarty::Range GetTinyRange(int64_t N) {
  if (N <= 0) return moriarty::EmptyRange();
  return Range(1, GetTinyMaximumThreshold(N));
}

moriarty::Range GetSmallRange(int64_t N) {
  if (N <= 0) return moriarty::EmptyRange();
  return Range(1, GetSmallMaximumThreshold(N));
}

moriarty::Range GetMediumRange(int64_t N) {
  if (N <= 0) return moriarty::EmptyRange();
  return Range(GetSmallMaximumThreshold(N), GetMediumMaximumThreshold(N));
}

moriarty::Range GetLargeRange(int64_t N) {
  if (N <= 0) return moriarty::EmptyRange();
  return Range(GetMediumMaximumThreshold(N), N);
}

moriarty::Range GetHugeRange(int64_t N) {
  if (N <= 0) return moriarty::EmptyRange();
  return Range(GetHugeMinimumThreshold(N), N);
}

moriarty::Range GetMaxRange(int64_t N) {
  if (N <= 0) return moriarty::EmptyRange();
  return Range(N, N);
}

std::string ToString(CommonSize size) {
  switch (size) {
    case CommonSize::kAny:
      return "any";
    case CommonSize::kMin:
      return "min";
    case CommonSize::kMax:
      return "max";
    case CommonSize::kSmall:
      return "small";
    case CommonSize::kMedium:
      return "medium";
    case CommonSize::kLarge:
      return "large";
    case CommonSize::kTiny:
      return "tiny";
    case CommonSize::kHuge:
      return "huge";
    default:
      ABSL_CHECK(false) << "Unknown size";
  }
}

CommonSize CommonSizeFromString(absl::string_view size) {
  if (size == "any") return CommonSize::kAny;
  if (size == "min") return CommonSize::kMin;
  if (size == "max") return CommonSize::kMax;
  if (size == "small") return CommonSize::kSmall;
  if (size == "medium") return CommonSize::kMedium;
  if (size == "large") return CommonSize::kLarge;
  if (size == "tiny") return CommonSize::kTiny;
  if (size == "huge") return CommonSize::kHuge;
  return CommonSize::kUnknown;
}

}  // namespace librarian
}  // namespace moriarty
