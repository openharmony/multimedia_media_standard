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

#ifndef FRAME_CONVERTER_H
#define FRAME_CONVERTER_H

#include <gst/gst.h>
#include "i_avmetadatahelper_service.h"
#include "frame_callback.h"
#include "inner_msg_define.h"
#include "gst_msg_processor.h"

namespace OHOS {
namespace Media {
class FrameConverter : public FrameCallback {
public:
    FrameConverter();
    ~FrameConverter() override;

    int32_t Init(const OutputConfiguration &config);
    void OnFrameAvaiable(GstBuffer &frame) override;
    int32_t StartConvert();
    std::shared_ptr<AVSharedMemory> GetOneFrame();
    int32_t StopConvert();
    int32_t Reset();

private:
    [[maybe_unused]] OutputConfiguration config_;
    [[maybe_unused]] GstPipeline *pipeline_ = nullptr;
    [[maybe_unused]] GstElement *appSink_ = nullptr;
    [[maybe_unused]] GstElement *appSrc_ = nullptr;
    [[maybe_unused]] std::unique_ptr<GstMsgProcessor> msgProcessor_;
};
}
}

#endif
