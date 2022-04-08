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

#ifndef AVMETA_FRAME_CONVERTER_H
#define AVMETA_FRAME_CONVERTER_H

#include <mutex>
#include <condition_variable>
#include <gst/gst.h>
#include "i_avmetadatahelper_service.h"
#include "avsharedmemory.h"
#include "inner_msg_define.h"
#include "gst_mem_sink.h"
#include "gst_msg_processor.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AVMetaFrameConverter : public NoCopyable {
public:
    AVMetaFrameConverter();
    ~AVMetaFrameConverter();

    int32_t Init(const OutputConfiguration &outConfig);
    std::shared_ptr<AVSharedMemory> Convert(GstCaps &inCaps, GstBuffer &inBuf);

private:
    int32_t SetupConvPipeline();
    int32_t SetupConvSrc();
    int32_t SetupConvSink(const OutputConfiguration &outConfig);
    int32_t SetupMsgProcessor();
    void UninstallPipeline();
    int32_t ChangeState(GstState targetState);
    int32_t PrepareConvert(GstCaps &inCaps);
    std::shared_ptr<AVSharedMemory> GetConvertResult();
    int32_t Reset();
    void OnNotifyMessage(const InnerMessage &msg);
    static GstFlowReturn OnNotifyNewSample(GstMemSink *elem, GstBuffer *sample, gpointer thiz);

    OutputConfiguration outConfig_;
    GstPipeline *pipeline_ = nullptr;
    GstElement *vidShMemSink_ = nullptr;
    GstElement *appSrc_ = nullptr;
    std::unique_ptr<GstMsgProcessor> msgProcessor_;
    GstState currState_ = GST_STATE_NULL;
    GstCaps *lastCaps_ = nullptr;
    GstBuffer *lastResult_ = nullptr;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool startConverting_ = false;
    std::vector<GstBuffer *> allResults_;
};
} // namespace Media
} // namespace OHOS
#endif // AVMETA_FRAME_CONVERTER_H
