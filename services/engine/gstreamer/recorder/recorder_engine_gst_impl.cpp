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

#include "recorder_engine_gst_impl.h"
#include "media_errors.h"
#include "media_log.h"
#include "recorder_private_param.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderEngineGstImpl"};
}

namespace OHOS {
namespace Media {
RecorderEngineGstImpl::RecorderEngineGstImpl()
{
    MEDIA_LOGD("enter, ctor");
    sourceCount_.resize(RECORDER_SOURCE_KIND_MAX);
}

RecorderEngineGstImpl::~RecorderEngineGstImpl()
{
    MEDIA_LOGD("enter, dtor");
    (void)Reset();
    MEDIA_LOGD("exit, dtor");
}

int32_t RecorderEngineGstImpl::Init()
{
    auto ctrler = std::make_shared<RecorderPipelineCtrler>();
    int32_t ret = ctrler->Init();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_INVALID_OPERATION);

    ctrler_ = ctrler;
    builder_ = std::make_unique<RecorderPipelineBuilder>();

    return MSERR_OK;
}

int32_t RecorderEngineGstImpl::SetVideoSource(VideoSourceType source, int32_t &sourceId)
{
    sourceId = INVALID_SOURCE_ID;

    if (source < VIDEO_SOURCE_SURFACE_YUV || source >= VIDEO_SOURCE_BUTT) {
        MEDIA_LOGE("Invalid video source type: %{public}d", source);
        return MSERR_INVALID_VAL;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    if (sourceCount_[RECORDER_SOURCE_KIND_VIDEO] >= VIDEO_SOURCE_MAX_COUNT)  {
        MEDIA_LOGE("No free video channel !");
        return MSERR_INVALID_OPERATION;
    }

    RecorderSourceDesc desc;
    desc.SetVideoSource(source, sourceCount_[RECORDER_SOURCE_KIND_VIDEO]);
    int32_t success = builder_->SetSource(desc);
    CHECK_AND_RETURN_RET(success == MSERR_OK, MSERR_INVALID_OPERATION);

    MEDIA_LOGI("add video source success, type: %{public}d", source);

    (void)allSources_.emplace(desc.handle_, desc);
    sourceCount_[RECORDER_SOURCE_KIND_VIDEO] += 1;

    sourceId = desc.handle_;
    return MSERR_OK;
}

int32_t RecorderEngineGstImpl::SetAudioSource(AudioSourceType source, int32_t &sourceId)
{
    sourceId = INVALID_SOURCE_ID;

    if (source <= AUDIO_SOURCE_INVALID || source > AUDIO_MIC) {
        MEDIA_LOGE("Input AudioSourceType : %{public}d is invalid", source);
        return MSERR_INVALID_VAL;
    }

    if (source == AudioSourceType::AUDIO_SOURCE_DEFAULT) {
        source = AudioSourceType::AUDIO_MIC;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    if (sourceCount_[RECORDER_SOURCE_KIND_AUDIO] >= AUDIO_SOURCE_MAX_COUNT)  {
        MEDIA_LOGE("No free audio channel !");
        return MSERR_INVALID_OPERATION;
    }

    RecorderSourceDesc desc;
    desc.SetAudioSource(source, static_cast<int32_t>(sourceCount_[RECORDER_SOURCE_KIND_AUDIO]));
    int32_t success = builder_->SetSource(desc);
    CHECK_AND_RETURN_RET(success == MSERR_OK, MSERR_INVALID_OPERATION);

    MEDIA_LOGI("add audios source success, type: %{public}d", source);

    (void)allSources_.emplace(desc.handle_, desc);
    sourceCount_[RECORDER_SOURCE_KIND_AUDIO] += 1;

    sourceId = desc.handle_;
    return MSERR_OK;
}

int32_t RecorderEngineGstImpl::SetOutputFormat(OutputFormatType format)
{
    if (format < FORMAT_DEFAULT || format >= FORMAT_BUTT) {
        MEDIA_LOGE("invalid output format: %{public}d", format);
        return MSERR_INVALID_VAL;
    }

    if (format == FORMAT_DEFAULT) {
        format = FORMAT_MPEG_4;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    if (allSources_.empty()) {
        MEDIA_LOGE("No source is set before set the output format!");
        return MSERR_INVALID_OPERATION;
    }

    int32_t ret = builder_->SetOutputFormat(format);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_INVALID_OPERATION);

    return MSERR_OK;
}

int32_t RecorderEngineGstImpl::BuildPipeline()
{
    pipeline_ = builder_->Build();
    CHECK_AND_RETURN_RET(pipeline_ != nullptr, MSERR_INVALID_OPERATION);

    ctrler_->SetPipeline(pipeline_);

    return MSERR_OK;
}

int32_t RecorderEngineGstImpl::SetObs(const std::weak_ptr<IRecorderEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    ctrler_->SetObs(obs);
    return MSERR_OK;
}

int32_t RecorderEngineGstImpl::Configure(int32_t sourceId, const RecorderParam &recParam)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if ((allSources_.find(sourceId) == allSources_.end()) && (sourceId != DUMMY_SOURCE_ID)) {
        MEDIA_LOGE("invalid sourceId: 0x%{public}x", sourceId);
        return MSERR_INVALID_OPERATION;
    }

    CHECK_AND_RETURN_RET(CheckParamType(sourceId, recParam), MSERR_INVALID_VAL);

    return builder_->Configure(sourceId, recParam);
}

sptr<Surface> RecorderEngineGstImpl::GetSurface(int32_t sourceId)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (allSources_.find(sourceId) == allSources_.end()) {
        MEDIA_LOGE("invalid sourceId: 0x%{public}x", sourceId);
        return nullptr;
    }

    if  (!allSources_[sourceId].IsVideo()) {
        MEDIA_LOGE("The sourceId %{public}d is not video source, GetSurface invalid !", sourceId);
        return nullptr;
    }

    if (pipeline_ == nullptr)  {
        MEDIA_LOGE("Pipeline is nullptr");
        return nullptr;
    }

    SurfaceParam param;
    int32_t ret = pipeline_->GetParameter(sourceId, param);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, nullptr);

    return param.surface_;
}

int32_t RecorderEngineGstImpl::Prepare()
{
    std::unique_lock<std::mutex> lock(mutex_);
    int32_t ret = BuildPipeline();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("Prepare failed due to pipeline build failed !");
        return ret;
    }

    ret = ctrler_->Prepare();
    pipeline_->Dump();

    return ret;
}

int32_t RecorderEngineGstImpl::Start()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return ctrler_->Start();
}

int32_t RecorderEngineGstImpl::Pause()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return ctrler_->Pause();
}

int32_t RecorderEngineGstImpl::Resume()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return ctrler_->Resume();
}

int32_t RecorderEngineGstImpl::Stop(bool isDrainAll)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (allSources_.empty())  {
        return MSERR_OK;
    }

    int ret = ctrler_->Stop(isDrainAll);

    (void)ctrler_->Reset();
    pipeline_ = nullptr;
    builder_->Reset();
    for (size_t i = 0; i < sourceCount_.size(); i++) {
        sourceCount_[i] = 0;
    }
    allSources_.clear();

    return ret;
}

int32_t RecorderEngineGstImpl::Reset()
{
    return Stop(false);
}

int32_t RecorderEngineGstImpl::SetParameter(int32_t sourceId, const RecorderParam &recParam)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if ((allSources_.find(sourceId) == allSources_.end()) && (sourceId != DUMMY_SOURCE_ID)) {
        MEDIA_LOGE("invalid sourceId: 0x%{public}x", sourceId);
        return MSERR_INVALID_OPERATION;
    }

    if (pipeline_ == nullptr)  {
        MEDIA_LOGE("Pipeline is nullptr");
        return MSERR_INVALID_STATE;
    }

    CHECK_AND_RETURN_RET(CheckParamType(sourceId, recParam), MSERR_INVALID_VAL);

    return pipeline_->SetParameter(sourceId, recParam);
}

bool RecorderEngineGstImpl::CheckParamType(int32_t sourceId, const RecorderParam &recParam) const
{
    if (sourceId == DUMMY_SOURCE_ID) {
        if (recParam.IsAudioParam() || recParam.IsVideoParam()) {
            MEDIA_LOGE("The specified param type is relevant with video or audio, but the source id is dummy");
            return false;
        }
        return true;
    }

    auto iter = allSources_.find(sourceId);
    if (iter == allSources_.end()) {
        MEDIA_LOGE("invalid sourceId: 0x%{public}x", sourceId);
        return false;
    }

    if (iter->second.IsVideo()) {
        if (recParam.IsVideoParam()) {
            return true;
        }
        MEDIA_LOGE("The specified sourceId is associated with video, but the param type is irrelevant with video !");
        return false;
    }
    if (iter->second.IsAudio()) {
        if (recParam.IsAudioParam()) {
            return true;
        }
        MEDIA_LOGE("The specified sourceId is associated with audio, but the param type is irrelevant with audio !");
        return false;
    }

    // unreachable.
    MEDIA_LOGE("unknown error !");
    return false;
}
}
}
