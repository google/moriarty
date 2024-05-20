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

#include "src/internal/random_config.h"

#include <cstdint>
#include <vector>

#include "absl/status/statusor.h"
#include "src/internal/random_engine.h"

namespace moriarty {
namespace moriarty_internal {

RandomConfig& RandomConfig::SetRandomEngine(RandomEngine* engine) {
  engine_ = engine;
  return *this;
}

RandomEngine* RandomConfig::GetRandomEngine() { return engine_; }

absl::StatusOr<int64_t> RandomInteger(RandomEngine& engine, int64_t min,
                                      int64_t max) {
  return engine.RandInt(min, max);
}

absl::StatusOr<int64_t> RandomInteger(RandomEngine& engine, int64_t n) {
  return RandomInteger(engine, 0, n - 1);
}

absl::StatusOr<std::vector<int>> RandomPermutation(RandomEngine& engine,
                                                   int n) {
  return RandomPermutation(engine, n, int{0});
}

}  // namespace moriarty_internal
}  // namespace moriarty
