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

#include "src/internal/status_utils.h"

#include <functional>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/strings/string_view.h"

namespace moriarty {
namespace moriarty_internal {

void TryFunctionOrCrash(std::function<absl::Status()> fn,
                        absl::string_view function_name_without_try) {
  absl::Status status = fn();
  if (status.ok()) return;

  ABSL_LOG(INFO)
      << "Encountered an error in " << function_name_without_try
      << ". If you would like to catch this error and not crash, use Try"
      << function_name_without_try << " instead.";
  ABSL_CHECK_OK(status);  // Should fail.
}

}  // namespace moriarty_internal
}  // namespace moriarty
