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

#ifndef MUX_SINK_BIN_H
#define MUX_SINK_BIN_H

#include <atomic>

#include "recorder_element.h"

namespace OHOS {
namespace Media {
class MuxSinkBin : public RecorderElement {
public:
    using RecorderElement::RecorderElement;
    ~MuxSinkBin();

    int32_t Init() override;
    int32_t Configure(const RecorderParam &recParam) override;
    int32_t CheckConfigReady() override;
    int32_t Prepare() override;
    int32_t Reset() override;
    int32_t SetParameter(const RecorderParam &recParam) override;
    void Dump() override;

private:
    int32_t ConfigureOutputFormat(const RecorderParam &recParam);
    int32_t ConfigureOutputTarget(const RecorderParam &recParam);
    int32_t ConfigureMaxDuration(const RecorderParam &recParam);
    int32_t ConfigureMaxFileSize(const RecorderParam &recParam);
    int32_t SetOutFilePath();
    int32_t CreateMuxerElement(const std::string &name);

    GstElement *gstMuxer_ = nullptr;
    GstElement *gstSink_ = nullptr;
    std::string outPath_;
    bool isReg_ = false;
    int outFd_ = -1;
    int32_t format_ = OutputFormatType::FORMAT_MPEG_4;
    int32_t maxDuration_ = -1;
    int64_t maxSize_ = -1;
};
}
}
#endif
