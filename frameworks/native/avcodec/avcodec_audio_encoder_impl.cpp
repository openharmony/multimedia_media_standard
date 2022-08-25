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

#include "avcodec_audio_encoder_impl.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecAudioEncoderImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<AVCodecAudioEncoder> AudioEncoderFactory::CreateByMime(const std::string &mime)
{
    std::shared_ptr<AVCodecAudioEncoderImpl> impl = std::make_shared<AVCodecAudioEncoderImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new AVCodecAudioEncoderImpl");

    int32_t ret = impl->Init(AVCODEC_TYPE_AUDIO_ENCODER, true, mime);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init AVCodecAudioEncoderImpl");

    return impl;
}

std::shared_ptr<AVCodecAudioEncoder> AudioEncoderFactory::CreateByName(const std::string &name)
{
    std::shared_ptr<AVCodecAudioEncoderImpl> impl = std::make_shared<AVCodecAudioEncoderImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new AVCodecAudioEncoderImpl");

    int32_t ret = impl->Init(AVCODEC_TYPE_AUDIO_ENCODER, false, name);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init AVCodecAudioEncoderImpl");

    return impl;
}

int32_t AVCodecAudioEncoderImpl::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    codecService_ = MediaServiceFactory::GetInstance().CreateAVCodecService();
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_UNKNOWN, "failed to create avcodec service");

    return codecService_->InitParameter(type, isMimeType, name);
}

AVCodecAudioEncoderImpl::AVCodecAudioEncoderImpl()
{
    MEDIA_LOGD("AVCodecAudioEncoderImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecAudioEncoderImpl::~AVCodecAudioEncoderImpl()
{
    if (codecService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyAVCodecService(codecService_);
        codecService_ = nullptr;
    }
    MEDIA_LOGD("AVCodecAudioEncoderImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecAudioEncoderImpl::Configure(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Configure(format);
}

int32_t AVCodecAudioEncoderImpl::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Prepare();
}

int32_t AVCodecAudioEncoderImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Start();
}

int32_t AVCodecAudioEncoderImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Stop();
}

int32_t AVCodecAudioEncoderImpl::Flush()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Flush();
}

int32_t AVCodecAudioEncoderImpl::Reset()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Reset();
}

int32_t AVCodecAudioEncoderImpl::Release()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Release();
}

std::shared_ptr<AVSharedMemory> AVCodecAudioEncoderImpl::GetInputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, nullptr, "service died");
    return codecService_->GetInputBuffer(index);
}

int32_t AVCodecAudioEncoderImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->QueueInputBuffer(index, info, flag);
}

std::shared_ptr<AVSharedMemory> AVCodecAudioEncoderImpl::GetOutputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, nullptr, "service died");
    return codecService_->GetOutputBuffer(index);
}

int32_t AVCodecAudioEncoderImpl::GetOutputFormat(Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->GetOutputFormat(format);
}

int32_t AVCodecAudioEncoderImpl::ReleaseOutputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->ReleaseOutputBuffer(index);
}

int32_t AVCodecAudioEncoderImpl::SetParameter(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->SetParameter(format);
}

int32_t AVCodecAudioEncoderImpl::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "callback is nullptr");
    return codecService_->SetCallback(callback);
}
} // namespace Media
} // namespace OHOS
