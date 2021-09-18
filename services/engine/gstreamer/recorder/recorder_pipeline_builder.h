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

#ifndef RECORDER_PIPELINE_BUILDER
#define RECORDER_PIPELINE_BUILDER

#include <memory>
#include "nocopyable.h"
#include "recorder_inner_defines.h"
#include "recorder_pipeline.h"
#include "recorder_element.h"
#include "recorder_pipeline_link_helper.h"

namespace OHOS {
namespace Media {
class RecorderPipelineBuilder {
public:
    RecorderPipelineBuilder();
    ~RecorderPipelineBuilder();

    int32_t SetSource(const RecorderSourceDesc &desc);
    int32_t SetOutputFormat(OutputFormatType formatType);
    int32_t Configure(int32_t sourceId, const RecorderParam &param);
    std::shared_ptr<RecorderPipeline> Build();

    void Reset();

    DISALLOW_COPY_AND_MOVE(RecorderPipelineBuilder);

private:
    int32_t SetVideoSource(const RecorderSourceDesc &desc);
    int32_t SetAudioSource(const RecorderSourceDesc &desc);
    int32_t CreateMuxSink();
    std::shared_ptr<RecorderElement> CreateElement(
        const std::string &name, const RecorderSourceDesc &desc, bool isSource);
    void EnsureSourceOrder(bool isVideo);

    int32_t ExecuteLink();

    std::shared_ptr<RecorderPipelineDesc> pipelineDesc_;
    std::shared_ptr<RecorderPipeline> pipeline_;
    std::shared_ptr<RecorderElement> muxSink_;
    bool outputFormatConfiged_ = false;
    std::unique_ptr<RecorderPipelineLinkHelper> linkHelper_;
    size_t videoSrcCount = 0;
    size_t otherSrcCount = 0;    
};
}
}
#endif
