/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_RECORDER_MESSAGE_PROCESSOR_H
#define OHOS_MEDIA_RECORDER_MESSAGE_PROCESSOR_H

#include <gst/gst.h>
#include <functional>
#include <vector>
#include <mutex>
#include "nocopyable.h"
#include "recorder_message_handler.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
using MessageResCb = std::function<void(const RecorderMessage&)>;

class RecorderMsgProcessor {
public:
    RecorderMsgProcessor(GstBus &gstBus, const MessageResCb &resCb);
    ~RecorderMsgProcessor();
    int32_t Init();
    int32_t Reset();
    void AddMsgHandler(std::shared_ptr<RecorderMsgHandler> handler);

private:
    static gboolean BusCallback(GstBus *bus, GstMessage *msg, gpointer data);
    void ProcessMessage(GstMessage &msg);
    RecorderMsgProcResult ProcessExtendMessage(GstMessage &msg, RecorderMessage &prettyMsg) const;
    RecorderMsgProcResult ProcessMessageFinal(GstMessage &msg, RecorderMessage &prettyMsg);
    void NotifyInternalError(RecorderMessage &msg);
    void ReportMsgProcResult(const RecorderMessage &msg);
    void ClearResource();

    DISALLOW_COPY_AND_MOVE(RecorderMsgProcessor);

    GstBus *gstBus_ = nullptr;
    GMainLoop *mainLoop_ = nullptr;
    TaskQueue mainLoopGuard_;
    guint busWatchId_ = 0;

    MessageResCb msgResultCb_;
    std::vector<std::shared_ptr<RecorderMsgHandler>> msgHandlers_;
    std::unique_ptr<TaskQueue> errorProcQ_;
    std::mutex mutex_;
};
}
}
#endif