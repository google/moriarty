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

#ifndef MORIARTY_SRC_UTIL_STATUS_MACRO_EXAMINESTACK_H_
#define MORIARTY_SRC_UTIL_STATUS_MACRO_EXAMINESTACK_H_

#include <string>

namespace moriarty {
namespace moriarty_status {

// Type of function used for printing in stack trace dumping, etc.
// We avoid closures to keep things simple.
typedef void DebugWriter(const char*, void*);
// Dump current stack trace omitting the topmost 'skip_count' stack frames.
void DumpStackTrace(int skip_count, DebugWriter* w, void* arg,
                    bool short_format = false);
// Dump given pc and stack trace.
void DumpPCAndStackTrace(void* pc, void* const stack[], int depth,
                         DebugWriter* writerfn, void* arg,
                         bool short_format = false);
// Return the current stack trace as a string (on multiple lines, beginning with
// "Stack trace:\n").
std::string CurrentStackTrace(bool short_format = false);

}  // namespace moriarty_status
}  // namespace moriarty

#endif  // MORIARTY_SRC_UTIL_STATUS_MACRO_EXAMINESTACK_H_
