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

#ifndef GST_MSG_PROCESSOR_H
#define GST_MSG_PROCESSOR_H

#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <gst/gst.h>
#include "inner_msg_define.h"
#include "task_queue.h"
#include "gst_msg_converter.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class GstMsgProcessor {
public:
    GstMsgProcessor(GstBus &gstBus, const InnerMsgNotifier &notifier,
        const std::shared_ptr<IGstMsgConverter> &converter = nullptr);
    ~GstMsgProcessor();

    int32_t Init();

    /**
     * Add the gst msg filter except the error msg.
     * The filter string will be compared with the gst msg src's name.
     * If matched, the msg will be notified, or the msg will be ignored.
     */
    void AddMsgFilter(const std::string &filter);
    void FlushBegin();
    void FlushEnd();
    void Reset() noexcept;

    DISALLOW_COPY_AND_MOVE(GstMsgProcessor);

private:
    int32_t DoInit();
    static gboolean MainLoopRunDone(GstMsgProcessor *thiz);
    static gboolean BusCallback(const GstBus *bus, GstMessage *msg, GstMsgProcessor *thiz);
    void ProcessGstMessage(GstMessage &msg);
    void DoReset();

    GstBus *gstBus_ = nullptr;
    GMainLoop *mainLoop_ = nullptr;
    GMainContext *context_ = nullptr;
    GSource *busSource_ = 0;
    InnerMsgNotifier notifier_;
    TaskQueue guardTask_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool needWaiting_ = true;
    std::shared_ptr<IGstMsgConverter> msgConverter_;
    std::vector<std::string> filters_;
};
}
}

#endif