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

#include "src/internal/generation_config.h"

#include <cstdint>
#include <deque>
#include <iterator>
#include <optional>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/meta/type_traits.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"

namespace moriarty {
namespace moriarty_internal {

absl::Status GenerationConfig::MarkStartGeneration(
    absl::string_view variable_name) {
  auto [it, inserted] =
      generation_info_.insert({std::string(variable_name), GenerationInfo{}});

  GenerationInfo& generation_info = it->second;
  if (generation_info.actively_being_generated) {
    return absl::FailedPreconditionError(absl::Substitute(
        "cyclic dependency while generating $0", variable_name));
  }
  generation_info.actively_being_generated = true;
  generation_info.generated_variables_size_before_generation =
      generated_variables_.size();

  variables_actively_being_generated_.push(
      {.variable_name = std::string(variable_name), .active_retry_count = 0});

  return absl::OkStatus();
}

absl::Status GenerationConfig::MarkSuccessfulGeneration(
    absl::string_view variable_name) {
  if (variables_actively_being_generated_.empty() ||
      variables_actively_being_generated_.top().variable_name !=
          variable_name) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Marking a successful generation for the wrong variable. Expected "
        "the variable to be: `$0`, but was `$1`",
        variables_actively_being_generated_.empty()
            ? "(empty)"
            : variables_actively_being_generated_.top().variable_name,
        variable_name));
  }

  variables_actively_being_generated_.pop();
  generated_variables_.push_back(std::string(variable_name));

  GenerationInfo& gen_info = generation_info_.find(variable_name)->second;
  gen_info.most_recent_generation_status = absl::OkStatus();
  gen_info.actively_being_generated = false;

  total_generate_calls_++;

  return absl::OkStatus();
}

absl::Status GenerationConfig::MarkAbandonedGeneration(
    absl::string_view variable_name) {
  if (variables_actively_being_generated_.empty() ||
      variables_actively_being_generated_.top().variable_name !=
          variable_name) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Marking an unsuccessful generation for the wrong variable. Expected "
        "the variable to be: `$0`, but was `$1`",
        variables_actively_being_generated_.empty()
            ? "(empty)"
            : variables_actively_being_generated_.top().variable_name,
        variable_name));
  }

  variables_actively_being_generated_.pop();

  GenerationInfo& gen_info = generation_info_.find(variable_name)->second;
  gen_info.actively_being_generated = false;

  return absl::OkStatus();
}

namespace {

// Resizes `vec` to `desired_size`, and returns a vector with the contents
// of that suffix.
std::vector<std::string> ExtractSuffix(std::vector<std::string>& vec,
                                       int desired_size) {
  std::vector<std::string> result(
      std::make_move_iterator(vec.begin() + desired_size),
      std::make_move_iterator(vec.end()));
  vec.resize(desired_size);
  return result;
}

}  // namespace

absl::StatusOr<GenerationConfig::RetryRecommendation>
GenerationConfig::AddGenerationFailure(absl::string_view variable_name,
                                       absl::Status status) {
  if (variables_actively_being_generated_.empty() ||
      variables_actively_being_generated_.top().variable_name !=
          variable_name) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Marking an unsuccessful generation for the wrong variable. Expected "
        "the variable to be: `$0`, but was `$1`",
        variables_actively_being_generated_.empty()
            ? "(empty)"
            : variables_actively_being_generated_.top().variable_name,
        variable_name));
  }
  if (status.ok()) {
    return absl::InvalidArgumentError(
        "Passed absl::OkStatus() to AddGenerationFailure()");
  }

  GenerationInfo& generation_info =
      generation_info_.find(variable_name)->second;
  generation_info.most_recent_generation_status = std::move(status);

  ActiveGenerationMetadata& metadata =
      variables_actively_being_generated_.top();

  int active_retries = ++metadata.active_retry_count;
  int total_retries = ++generation_info.total_retry_count;
  int total_generates = ++total_generate_calls_;

  RetryRecommendation recommendation = {
      .variable_names_to_delete = ExtractSuffix(
          generated_variables_,
          generation_info.generated_variables_size_before_generation)};

  if (active_retries > kMaxActiveRetries || total_retries > kMaxTotalRetries ||
      total_generates > kMaxTotalGenerateCalls) {
    recommendation.policy = RetryRecommendation::kAbort;
    return recommendation;
  }

  recommendation.policy = RetryRecommendation::kRetry;
  return recommendation;
}

absl::Status GenerationConfig::GetGenerationStatus(
    absl::string_view variable_name) {
  auto it = generation_info_.find(variable_name);

  if (it == generation_info_.end()) {
    return absl::InvalidArgumentError(absl::Substitute(
        "No generation status available for '$0'", variable_name));
  }

  std::optional<absl::Status> status = it->second.most_recent_generation_status;
  if (!status.has_value()) {
    return absl::FailedPreconditionError(absl::Substitute(
        "No generation status available for '$0'", variable_name));
  }

  return *status;
}

void GenerationConfig::SetSoftGenerationLimit(int64_t limit) {
  soft_generation_limit_ = limit;
}

std::optional<int64_t> GenerationConfig::GetSoftGenerationLimit() const {
  return soft_generation_limit_;
}

}  // namespace moriarty_internal
}  // namespace moriarty
