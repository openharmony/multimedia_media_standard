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

#include "avcodec_engine_gst_impl.h"
#include "avcodeclist_engine_gst_impl.h"
#include "media_errors.h"
#include "media_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecEngineGstImpl"};
}

namespace OHOS {
namespace Media {
AVCodecEngineGstImpl::AVCodecEngineGstImpl()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecEngineGstImpl::~AVCodecEngineGstImpl()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (ctrl_ != nullptr) {
        (void)ctrl_->Release();
    }
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecEngineGstImpl::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    MEDIA_LOGD("Init AVCodecGstEngine: type:%{public}d, %{public}d, name:%{public}s", type, isMimeType, name.c_str());

    type_ = type;

    CodecMimeType codecName = CODEC_MIMIE_TYPE_VIDEO_H263;
    CHECK_AND_RETURN_RET(MapCodecMime(name, codecName) == MSERR_OK, MSERR_UNKNOWN);

    processor_ = AVCodecEngineFactory::CreateProcessor(type);
    CHECK_AND_RETURN_RET(processor_ != nullptr, MSERR_NO_MEMORY);
    CHECK_AND_RETURN_RET(processor_->Init(codecName) == MSERR_OK, MSERR_UNKNOWN);

    ctrl_ = std::make_unique<AVCodecEngineCtrl>();
    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    ctrl_->SetObs(obs_);

    if (isMimeType) {
        CHECK_AND_RETURN_RET(HandleMimeType(type, name) == MSERR_OK, MSERR_UNKNOWN);
    } else {
        CHECK_AND_RETURN_RET(HandlePluginName(type, name) == MSERR_OK, MSERR_UNKNOWN);
    }

    return MSERR_OK;
}

int32_t AVCodecEngineGstImpl::Configure(const Format &format)
{
    MEDIA_LOGD("Enter Configure");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(processor_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(processor_->DoProcess(format) == MSERR_OK, MSERR_UNKNOWN);

    MEDIA_LOGD("Configure success");
    return MSERR_OK;
}

int32_t AVCodecEngineGstImpl::Prepare()
{
    MEDIA_LOGD("Enter Prepare");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(processor_ != nullptr, MSERR_UNKNOWN);
    auto inputConfig = processor_->GetInputPortConfig();
    CHECK_AND_RETURN_RET(inputConfig != nullptr, MSERR_NO_MEMORY);

    auto outputConfig = processor_->GetOutputPortConfig();
    CHECK_AND_RETURN_RET(outputConfig != nullptr, MSERR_NO_MEMORY);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_UNKNOWN);
    return ctrl_->Prepare(inputConfig, outputConfig);
}

int32_t AVCodecEngineGstImpl::Start()
{
    MEDIA_LOGD("Enter Start");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_UNKNOWN);
    return ctrl_->Start();
}

int32_t AVCodecEngineGstImpl::Stop()
{
    MEDIA_LOGD("Enter Stop");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_UNKNOWN);
    return ctrl_->Stop();
}

int32_t AVCodecEngineGstImpl::Flush()
{
    MEDIA_LOGD("Enter Flush");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_UNKNOWN);
    return ctrl_->Flush();
}

int32_t AVCodecEngineGstImpl::Reset()
{
    MEDIA_LOGD("Enter Reset");
    std::unique_lock<std::mutex> lock(mutex_);
    if (ctrl_ != nullptr) {
        (void)ctrl_->Release();
        ctrl_ = nullptr;
    }
    ctrl_ = std::make_unique<AVCodecEngineCtrl>();
    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    CHECK_AND_RETURN_RET(ctrl_->Init(type_, uswSoftWare_, pluginName_) == MSERR_OK, MSERR_UNKNOWN);

    MEDIA_LOGD("Reset success");
    return MSERR_OK;
}

sptr<Surface> AVCodecEngineGstImpl::CreateInputSurface()
{
    MEDIA_LOGD("Enter CreateInputSurface");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(processor_ != nullptr, nullptr);
    auto inputConfig = processor_->GetInputPortConfig();
    CHECK_AND_RETURN_RET(inputConfig != nullptr, nullptr);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, nullptr);
    return ctrl_->CreateInputSurface(inputConfig);
}

int32_t AVCodecEngineGstImpl::SetOutputSurface(sptr<Surface> surface)
{
    MEDIA_LOGD("Enter SetOutputSurface");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    return ctrl_->SetOutputSurface(surface);
}

std::shared_ptr<AVSharedMemory> AVCodecEngineGstImpl::GetInputBuffer(uint32_t index)
{
    MEDIA_LOGD("Enter GetInputBuffer, index:%{public}d", index);
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, nullptr);
    return ctrl_->GetInputBuffer(index);
}

int32_t AVCodecEngineGstImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    MEDIA_LOGD("Enter QueueInputBuffer, index:%{public}d", index);
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    return ctrl_->QueueInputBuffer(index, info, flag);
}

std::shared_ptr<AVSharedMemory> AVCodecEngineGstImpl::GetOutputBuffer(uint32_t index)
{
    MEDIA_LOGD("Enter GetOutputBuffer, index:%{public}d", index);
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, nullptr);
    return ctrl_->GetOutputBuffer(index);
}

int32_t AVCodecEngineGstImpl::GetOutputFormat(Format &format)
{
    return MSERR_OK;
}

std::shared_ptr<AudioCaps> AVCodecEngineGstImpl::GetAudioCaps()
{
    return nullptr;
}

std::shared_ptr<VideoCaps> AVCodecEngineGstImpl::GetVideoCaps()
{
    return nullptr;
}

int32_t AVCodecEngineGstImpl::ReleaseOutputBuffer(uint32_t index, bool render)
{
    MEDIA_LOGD("Enter ReleaseOutputBuffer, index:%{public}d", index);
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    return ctrl_->ReleaseOutputBuffer(index, render);
}

int32_t AVCodecEngineGstImpl::SetParameter(const Format &format)
{
    MEDIA_LOGD("Enter SetParameter");
    std::unique_lock<std::mutex> lock(mutex_);

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_NO_MEMORY);
    return ctrl_->SetParameter(format);
}

int32_t AVCodecEngineGstImpl::SetObs(const std::weak_ptr<IAVCodecEngineObs> &obs)
{
    std::unique_lock<std::mutex> lock(mutex_);
    obs_ = obs;
    return MSERR_OK;
}

int32_t AVCodecEngineGstImpl::HandleMimeType(AVCodecType type, const std::string &name)
{
    int32_t ret = MSERR_OK;
    auto codecList = std::make_unique<AVCodecListEngineGstImpl>();
    CHECK_AND_RETURN_RET(codecList != nullptr, MSERR_NO_MEMORY);

    std::string pluginName = "";
    Format format;
    format.PutStringValue("codec_mime", name);
    switch (type) {
        case AVCODEC_TYPE_VIDEO_ENCODER:
            pluginName = codecList->FindVideoEncoder(format);
            break;
        case AVCODEC_TYPE_VIDEO_DECODER:
            pluginName = codecList->FindVideoDecoder(format);
            break;
        case AVCODEC_TYPE_AUDIO_ENCODER:
            pluginName = codecList->FindAudioEncoder(format);
            break;
        case AVCODEC_TYPE_AUDIO_DECODER:
            pluginName = codecList->FindAudioDecoder(format);
            break;
        default:
            ret = MSERR_INVALID_VAL;
            MEDIA_LOGE("Unknown type");
            break;
    }
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_UNKNOWN);
    MEDIA_LOGD("Found plugin name:%{public}s", pluginName.c_str());

    bool isSoftware = true;
    (void)QueryIsSoftPlugin(pluginName, isSoftware);

    uswSoftWare_ = isSoftware;
    pluginName_ = pluginName;

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_UNKNOWN);
    return ctrl_->Init(type, isSoftware, pluginName);
}

int32_t AVCodecEngineGstImpl::HandlePluginName(AVCodecType type, const std::string &name)
{
    bool isSoftware = true;
    (void)QueryIsSoftPlugin(name, isSoftware);

    uswSoftWare_ = isSoftware;
    pluginName_ = name;

    CHECK_AND_RETURN_RET(ctrl_ != nullptr, MSERR_UNKNOWN);
    return ctrl_->Init(type, isSoftware, name);
}

int32_t AVCodecEngineGstImpl::QueryIsSoftPlugin(const std::string &name, bool &isSoftware)
{
    auto codecList = std::make_unique<AVCodecListEngineGstImpl>();
    CHECK_AND_RETURN_RET(codecList != nullptr, MSERR_NO_MEMORY);

    std::vector<CapabilityData> data = codecList->GetCodecCapabilityInfos();
    bool pluginExist = false;

    for (auto it = data.begin(); it != data.end(); it++) {
        if ((*it).codecName == name) {
            isSoftware = !(*it).isVendor;
            pluginExist = true;
        }
    }
    CHECK_AND_RETURN_RET(pluginExist == true, MSERR_INVALID_VAL);

    return MSERR_OK;
}
} // Media
} // OHOS
