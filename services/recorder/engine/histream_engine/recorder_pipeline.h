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

#ifndef RECORDER_PIPELINE
#define RECORDER_PIPELINE

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <condition_variable>
#include <mutex>
#include <map>
#include <gst/gst.h>
#include "nocopyable.h"
#include "recorder.h"
#include "recorder_inner_defines.h"
#include "recorder_param.h"
#include "recorder_element.h"
#include "task_queue.h"

namespace OHOS {
namespace Media {
struct RecorderPipelineDesc {
    struct LinkDesc {
        std::shared_ptr<RecorderElement> dstElem;
        std::string srcPad;
        std::string sinkPad;
        bool isSrcPadStatic;
        bool isSinkPadStatic;
    };

    std::vector<std::shared_ptr<RecorderElement>> allElems;
    std::map<int32_t, std::shared_ptr<RecorderElement>> srcElems;
    std::shared_ptr<RecorderElement> muxerSinkBin;
    std::map<std::shared_ptr<RecorderElement>, std::vector<LinkDesc>> allLinkDescs;
};

class RecorderPipeline {
public:
    explicit RecorderPipeline(std::shared_ptr<RecorderPipelineDesc> desc);
    ~RecorderPipeline();

    int32_t Init();
    int32_t Prepare();
    int32_t Start();
    int32_t Pause();
    int32_t Resume();
    int32_t Stop(bool isDrainAll);
    int32_t Reset();
    int32_t SetParameter(int32_t sourceId, const RecorderParam &recParam);
    int32_t GetParameter(int32_t sourceId, RecorderParam &recParam);
    void SetNotifier(RecorderNotifier notifier);

    DISALLOW_COPY_AND_MOVE(RecorderPipeline);

private:
    using ElemAction = std::function<int32_t(RecorderElement&)>;
    int32_t DoElemAction(const ElemAction &action, bool needAllSucc = true);
    int32_t SyncWaitChangeState(GstState targetState);
    int32_t PostAndSyncWaitEOS();
    void SyncWaitEOS();
    void DrainBuffer(bool isDrainAll);
    void ClearResource();
    static gboolean BusCallbackWrapper(GstBus *bus, const GstMessage *msg, gpointer data);
    int32_t BusCallback(const GstMessage &msg);
    void NotifyCallback(int32_t type, int32_t code, int32_t detail);

    friend class RecorderPipelineLinkHelper;

    std::shared_ptr<RecorderPipelineDesc> desc_;
    RecorderNotifier notifier_;
    GstPipeline *gstPipeline_ = nullptr;
    GMainLoop *mainLoop_ = nullptr;
    guint busWatchId_ = 0;

    std::condition_variable gstPipeCond_;
    std::mutex gstPipeMutex_;
    bool asyncDone_ = false;
    bool eosDone_ = false;
    TaskQueue mainLoopGuard_;
    bool isStarted_ = false;
    GstState currState_ = GST_STATE_NULL;
};
}
}
#endif