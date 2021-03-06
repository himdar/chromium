// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feedback/tracing_manager.h"

#include "base/bind.h"
#include "base/file_util.h"
#include "base/location.h"
#include "base/memory/ref_counted_memory.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/prefs/pref_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/feedback/feedback_util.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/tracing_controller.h"

namespace {
// Only once trace manager can exist at a time.
TracingManager* g_tracing_manager = NULL;
// Trace IDs start at 1 and increase.
int g_next_trace_id = 1;
// Name of the file to store the tracing data as.
const base::FilePath::CharType kTracingFilename[] =
    FILE_PATH_LITERAL("tracing.json");
}

TracingManager::TracingManager()
    : current_trace_id_(0),
      weak_ptr_factory_(this) {
  DCHECK(!g_tracing_manager);
  g_tracing_manager = this;
  StartTracing();
}

TracingManager::~TracingManager() {
  DCHECK(g_tracing_manager == this);
  g_tracing_manager = NULL;
}

int TracingManager::RequestTrace() {
  // Return the current trace if one is being collected.
  if (current_trace_id_)
    return current_trace_id_;

  current_trace_id_ = g_next_trace_id;
  ++g_next_trace_id;
  content::TracingController::GetInstance()->DisableRecording(
      base::FilePath(),
      base::Bind(&TracingManager::OnTraceDataCollected,
                 weak_ptr_factory_.GetWeakPtr()));
  return current_trace_id_;
}

bool TracingManager::GetTraceData(int id, const TraceDataCallback& callback) {
  // If a trace is being collected currently, send it via callback when
  // complete.
  if (current_trace_id_) {
    // Only allow one trace data request at a time.
    if (trace_callback_.is_null()) {
      trace_callback_ = callback;
      return true;
    } else {
      return false;
    }
  } else {
    std::map<int, scoped_refptr<base::RefCountedString> >::iterator data =
        trace_data_.find(id);
    if (data == trace_data_.end())
      return false;

    // Always return the data asychronously, so the behavior is consistant.
    base::MessageLoopProxy::current()->PostTask(
        FROM_HERE,
        base::Bind(callback, data->second));
    return true;
  }
}

void TracingManager::DiscardTraceData(int id) {
  trace_data_.erase(id);

  // If the trace is discarded before it is complete, clean up the accumulators.
  if (id == current_trace_id_) {
    current_trace_id_ = 0;

    // If the trace has already been requested, provide an empty string.
    if (!trace_callback_.is_null()) {
      trace_callback_.Run(scoped_refptr<base::RefCountedString>());
      trace_callback_.Reset();
    }
  }
}

void TracingManager::StartTracing() {
  content::TracingController::GetInstance()->EnableRecording(
      "", content::TracingController::DEFAULT_OPTIONS,
      content::TracingController::EnableRecordingDoneCallback());
}

void TracingManager::OnTraceDataCollected(const base::FilePath& path) {
  if (!current_trace_id_)
    return;

  std::string data;
  if (!base::ReadFileToString(path, &data)) {
    LOG(ERROR) << "Failed to read trace data from: " << path.value();
    return;
  }
  base::DeleteFile(path, false);

  std::string output_val;
  feedback_util::ZipString(
      base::FilePath(kTracingFilename), data, &output_val);

  scoped_refptr<base::RefCountedString> output(
      base::RefCountedString::TakeString(&output_val));

  trace_data_[current_trace_id_] = output;

  if (!trace_callback_.is_null()) {
    trace_callback_.Run(output);
    trace_callback_.Reset();
  }

  current_trace_id_ = 0;

  // Tracing has to be restarted asynchronous, so the TracingController can
  // clean up.
  base::MessageLoopProxy::current()->PostTask(
      FROM_HERE,
      base::Bind(&TracingManager::StartTracing,
                 weak_ptr_factory_.GetWeakPtr()));
}

// static
scoped_ptr<TracingManager> TracingManager::Create() {
  if (g_tracing_manager)
    return scoped_ptr<TracingManager>();
  return scoped_ptr<TracingManager>(new TracingManager());
}

TracingManager* TracingManager::Get() {
  return g_tracing_manager;
}
