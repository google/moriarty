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

#include "src/util/status_macro/status_builder.h"

#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "absl/base/log_severity.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "src/util/status_macro/examine_stack.h"

namespace moriarty {
namespace moriarty_status {

StatusBuilder::StatusBuilder() {}

StatusBuilder::Rep::Rep(const absl::Status& s) : status(s) {}

StatusBuilder::Rep::Rep(absl::Status&& s) : status(std::move(s)) {}

StatusBuilder::Rep::~Rep() {}

StatusBuilder::Rep::Rep(const Rep& r)
    : status(r.status),
      logging_mode(r.logging_mode),
      log_severity(r.log_severity),
      verbose_level(r.verbose_level),
      n(r.n),
      period(r.period),
      stream_message(r.stream_message),
      stream(&stream_message),
      should_log_stack_trace(r.should_log_stack_trace),
      message_join_style(r.message_join_style) {}

namespace {

static void CopyPayloads(absl::Status* dst, const absl::Status& src) {
  src.ForEachPayload([&](absl::string_view type_url, absl::Cord payload) {
    dst->SetPayload(type_url, payload);
  });
}

}  // namespace

absl::Status StatusBuilder::JoinMessageToStatus(absl::Status s,
                                                std::string_view msg,
                                                MessageJoinStyle style) {
  if (s.ok() || msg.empty()) return s;
  std::string new_msg;
  if (style == MessageJoinStyle::kAnnotate) {
    std::string formatted_msg{msg};
    if (!s.message().empty()) {
      new_msg = absl::StrCat(s.message(), "; ", formatted_msg);
    } else {
      new_msg = formatted_msg;
    }
  } else if (style == MessageJoinStyle::kPrepend) {
    new_msg = absl::StrCat(msg, s.message());
  } else {
    new_msg = absl::StrCat(s.message(), msg);
  }
  absl::Status result(s.code(), new_msg);
  CopyPayloads(&result, s);
  return result;
}

void StatusBuilder::ConditionallyLog(const absl::Status& status) const {
  if (rep_->logging_mode == Rep::LoggingMode::kDisabled) return;
  absl::LogSeverity severity = rep_->log_severity;
  switch (rep_->logging_mode) {
    case Rep::LoggingMode::kVLog:
    case Rep::LoggingMode::kDisabled:
    case Rep::LoggingMode::kLog:
      break;
    case Rep::LoggingMode::kLogEveryN: {
      {
        struct LogSites {
          absl::Mutex mutex;
          absl::flat_hash_map<std::pair<const void*, uint>, uint>
              counts_by_file_and_line ABSL_GUARDED_BY(mutex);
        };

        static auto* log_every_n_sites = new LogSites();
        absl::MutexLock lock(&log_every_n_sites->mutex);
        const uint count =
            log_every_n_sites
                ->counts_by_file_and_line[{loc_.file_name(), loc_.line()}]++;
        if (count % rep_->n != 0) {
          return;
        }
        break;
      }
    }
  }
  const std::string maybe_stack_trace =
      rep_->should_log_stack_trace ? absl::StrCat(" ", CurrentStackTrace())
                                   : "";
  LOG(LEVEL(severity)).AtLocation(loc_.file_name(), loc_.line())
      << status << maybe_stack_trace;
}

absl::Status StatusBuilder::CreateStatusAndConditionallyLog() && {
  absl::Status result = JoinMessageToStatus(
      std::move(rep_->status), rep_->stream_message, rep_->message_join_style);
  ConditionallyLog(result);
  // We consumed the status above, we set it to some error just to prevent
  // people relying on it become OK or something.
  rep_->status = absl::UnknownError("");
  rep_ = nullptr;
  return result;
}

std::ostream& operator<<(std::ostream& os, const StatusBuilder& builder) {
  return os << static_cast<absl::Status>(builder);
}

std::ostream& operator<<(std::ostream& os, StatusBuilder&& builder) {
  return os << static_cast<absl::Status>(std::move(builder));
}

}  // namespace moriarty_status
}  // namespace moriarty
