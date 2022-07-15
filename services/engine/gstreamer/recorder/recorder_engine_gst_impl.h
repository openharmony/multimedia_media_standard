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

#ifndef RECORDER_ENGINE_GST_IMPL_H
#define RECORDER_ENGINE_GST_IMPL_H

#include <map>
#include <vector>
#include <mutex>

#include "nocopyable.h"
#include "i_recorder_engine.h"
#include "recorder_pipeline.h"
#include "recorder_pipeline_ctrler.h"
#include "recorder_pipeline_builder.h"
#include "recorder_inner_defines.h"

namespace OHOS {
namespace Media {
class RecorderEngineGstImpl : public IRecorderEngine, public NoCopyable {
public:
    RecorderEngineGstImpl(int32_t appUid, int32_t appPid, uint32_t appTokenId);
    ~RecorderEngineGstImpl();

    int32_t Init();
    int32_t SetVideoSource(VideoSourceType source, int32_t &sourceId) override;
    int32_t SetAudioSource(AudioSourceType source, int32_t &sourceId) override;
    int32_t SetOutputFormat(OutputFormatType format) override;
    int32_t SetObs(const std::weak_ptr<IRecorderEngineObs> &obs) override;
    int32_t Configure(int32_t sourceId, const RecorderParam &recParam) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Pause() override;
    int32_t Resume() override;
    int32_t Stop(bool isDrainAll) override;
    int32_t Reset() override;
    int32_t SetParameter(int32_t sourceId, const RecorderParam &recParam) override;
    sptr<Surface> GetSurface(int32_t sourceId) override;

private:
    int32_t BuildPipeline();
    bool CheckParamType(int32_t sourceId, const RecorderParam &recParam) const;

    std::unique_ptr<RecorderPipelineBuilder> builder_ = nullptr;
    std::shared_ptr<RecorderPipelineCtrler> ctrler_ = nullptr;
    std::shared_ptr<RecorderPipeline> pipeline_ = nullptr;
    std::map<int32_t, RecorderSourceDesc> allSources_;
    std::vector<int32_t> sourceCount_;
    std::mutex mutex_;
    int32_t appUid_;
    int32_t appPid_;
    uint32_t appTokenId_;
};
} // namespace Media
} // namespace OHOS
#endif
