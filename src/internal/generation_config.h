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

#ifndef MORIARTY_SRC_INTERNAL_GENERATION_CONFIG_H_
#define MORIARTY_SRC_INTERNAL_GENERATION_CONFIG_H_

#include <cstdint>
#include <optional>
#include <stack>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

namespace moriarty {
namespace moriarty_internal {

// TODO(darcybest): Rename to GenerationHandler or similar.

// GenerationConfig()
//
// Maintains the list of variables that are being generated via Moriarty. As you
// generate variables, it must be in a stack-based order (the stack comes from
// dependent variables and subvariables).
//
// You must StartGeneration on a variable, which adds it to the stack.
// Then you must MarkSuccessfulGeneration or AbandonGeneration to pop it.
class GenerationConfig {
 public:
  // TODO(darcybest): Consider letting the user specify these values.
  constexpr static int kMaxActiveRetries = 1000;
  constexpr static int kMaxTotalRetries = 100000;
  constexpr static int64_t kMaxTotalGenerateCalls = 10 * 1000 * 1000;

  // MarkStartGeneration()
  //
  // Informs this class that `variable_name` has started generation.
  //
  // Returns a FailedPrecondition status if this variable is already in the
  // process of being generated (cyclic dependency).
  absl::Status MarkStartGeneration(absl::string_view variable_name);

  // MarkSuccessfulGeneration()
  //
  // Informs this class that `variable_name` has succeeded in its generation.
  //
  // MarkStartGeneration(variable_name) must have been called and all generation
  // attempts for all other variables since must be complete.
  absl::Status MarkSuccessfulGeneration(absl::string_view variable_name);

  // MarkAbandonedGeneration()
  //
  // Informs this class that `variable_name` has stopped attempting to generate
  // a value.
  //
  // MarkStartGeneration(variable_name) must have been called and all generation
  // attempts for all other variables since must be complete.
  absl::Status MarkAbandonedGeneration(absl::string_view variable_name);

  // RetryRecommendation (class)
  //
  // On a failed generation attempt, this recommends if you should retry to not.
  // Also, provides information about values of variables that should be deleted
  // from the universe.
  struct RetryRecommendation {
    enum Policy {
      kAbort,  // Do not continue retrying
      kRetry   // Retry generation
    };
    Policy policy;
    std::vector<std::string> variable_names_to_delete;
  };

  // AddGenerationFailure()
  //
  // Informs this class that `variable_name` has failed to generate a value with
  // a status of `failure_status`. Returns a recommendation for if the variable
  // should retry generation or abort generation.
  //
  // The list of variables to be deleted in the recommendation are those that
  // were generated since this variable started its generation. This class will
  // assume that the value for those variables have been deleted from the
  // universe.
  //
  // MarkStartGeneration(variable_name) must have been called and all generation
  // attempts for all other variables since must be complete.
  absl::StatusOr<RetryRecommendation> AddGenerationFailure(
      absl::string_view variable_name, absl::Status failure_status);

  // GetGenerationStatus()
  //
  // Returns the most recent generation status for `variable_name`. If no
  // attempts to generate were made, returns absl::InvalidArgumentError.
  //
  // `MarkStartGeneration(variable_name)` must have been called previously.
  absl::Status GetGenerationStatus(absl::string_view variable_name);

  // SetSoftGenerationLimit()
  //
  // The soft generation limit is the approximate upper bound on the total size
  // of all objects generated. No type in Moriarty is required to adhere to this
  // constraint. This limit is approximately weighted by the following basic
  // heuristics:
  //
  // * The size of an array is the sum of the GenSizes of the elements.
  // * The size of a string is the length of the string.
  // * All others are of size 1.
  //
  // The exact values used here should not be depended on, they may change at
  // any point.
  void SetSoftGenerationLimit(int64_t limit);

  // GetSoftGenerationLimit()
  //
  // Returns the soft generation limit. See SetSoftGenerationLimit for info.
  // If there is no soft limit, returns `std::nullopt`.
  std::optional<int64_t> GetSoftGenerationLimit() const;

 private:
  int64_t total_generate_calls_ = 0;

  // Metadata related to variables that are actively being generated.
  struct ActiveGenerationMetadata {
    std::string variable_name;
    int64_t active_retry_count = 0;
  };

  // Variables are generated in a stack-like fashion.
  std::stack<ActiveGenerationMetadata> variables_actively_being_generated_;

  // The variables that were successfully generated, in the order they were
  // generated.
  std::vector<std::string> generated_variables_;

  // Retry counts.
  //  * "Active" is set to 0 whenever MarkXXXXGeneration() is called.
  //  * "Total" is never reset
  struct GenerationInfo {
    std::optional<absl::Status> most_recent_generation_status;
    int total_retry_count = 0;
    bool actively_being_generated = false;
    int generated_variables_size_before_generation;
  };
  absl::flat_hash_map<std::string, GenerationInfo> generation_info_;

  std::optional<int64_t> soft_generation_limit_;
};

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_GENERATION_CONFIG_H_
