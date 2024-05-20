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

#ifndef MORIARTY_SRC_INTERNAL_STATUS_UTILS_H_
#define MORIARTY_SRC_INTERNAL_STATUS_UTILS_H_

#include <functional>

#include "absl/base/nullability.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

namespace moriarty {
namespace moriarty_internal {

// TryFunctionOrCrash() [absl::Status version]
//
// Evaluates `fn`, which returns `absl::Status`. If the status is
// `ok`, then this does nothing. Otherwise, this crashes the program with a
// slightly nicer error message than just ABSL_CHECK_OK provides (and then calls
// ABSL_CHECK_OK).
//
// This assumes that you are in a function named `Foo()` and there is a
// `TryFoo()` available for the user to call.
void TryFunctionOrCrash(std::function<absl::Status()> fn,
                        absl::string_view function_name_without_try);

// TryFunctionOrCrash() [absl::StatusOr<> version]
//
// Evaluates `fn`, which returns `absl::StatusOr`. If the status
// is `ok`, then this returns the underlying value. Otherwise, crashes the
// program with a slightly nicer error message than just ABSL_CHECK_OK provides
// (and then calls ABSL_CHECK_OK).
//
// This assumes that you are in a function named `Foo()` and there is a
// `TryFoo()` available for the user to call.
template <typename T>
T TryFunctionOrCrash(std::function<absl::StatusOr<T>()> fn,
                     absl::string_view function_name_without_try) {
  absl::StatusOr<T> status_or = fn();
  if (status_or.ok()) return *status_or;

  ABSL_LOG(INFO)
      << "Encountered an error in " << function_name_without_try
      << ". If you would like to catch this error and not crash, use Try"
      << function_name_without_try << " instead.";
  ABSL_CHECK_OK(status_or);

  // This line will not be invoked, but including to avoid lint errors.
  return *status_or;
}

// TryFunctionOrCrash() [absl::StatusOr<T*> version]
//
// Evaluates `fn`, which returns `absl::StatusOr`. If the status
// is `ok`, then this returns the underlying value. Otherwise, crashes the
// program with a slightly nicer error message than just ABSL_CHECK_OK provides
// (and then calls ABSL_CHECK_OK).
//
// This assumes that you are in a function named `Foo()` which returns a `T&`,
// and there is a `TryFoo()` that returns `absl::StatusOr<absl::Nonnull<T*>>`
// available for the user to call.
template <typename T>
T& TryFunctionOrCrash(std::function<absl::StatusOr<absl::Nonnull<T*>>()> fn,
                      absl::string_view function_name_without_try) {
  absl::StatusOr<absl::Nonnull<T*>> status_or = fn();
  if (status_or.ok()) return *(*status_or);

  ABSL_LOG(INFO)
      << "Encountered an error in " << function_name_without_try
      << ". If you would like to catch this error and not crash, use Try"
      << function_name_without_try << " instead.";
  ABSL_CHECK_OK(status_or);

  // This line will not be invoked, but including to avoid lint errors.
  return *(*status_or);
}

}  // namespace moriarty_internal
}  // namespace moriarty

#endif  // MORIARTY_SRC_INTERNAL_STATUS_UTILS_H_
