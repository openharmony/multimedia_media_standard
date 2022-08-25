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

#include "avcodec_audio_decoder_impl.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecAudioDecoderImpl"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<AVCodecAudioDecoder> AudioDecoderFactory::CreateByMime(const std::string &mime)
{
    std::shared_ptr<AVCodecAudioDecoderImpl> impl = std::make_shared<AVCodecAudioDecoderImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new AVCodecAudioDecoderImpl");

    int32_t ret = impl->Init(AVCODEC_TYPE_AUDIO_DECODER, true, mime);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init AVCodecAudioDecoderImpl");

    return impl;
}

std::shared_ptr<AVCodecAudioDecoder> AudioDecoderFactory::CreateByName(const std::string &name)
{
    std::shared_ptr<AVCodecAudioDecoderImpl> impl = std::make_shared<AVCodecAudioDecoderImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new AVCodecAudioDecoderImpl");

    int32_t ret = impl->Init(AVCODEC_TYPE_AUDIO_DECODER, false, name);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init AVCodecAudioDecoderImpl");

    return impl;
}

int32_t AVCodecAudioDecoderImpl::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    codecService_ = MediaServiceFactory::GetInstance().CreateAVCodecService();
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_UNKNOWN, "failed to create avcodec service");

    return codecService_->InitParameter(type, isMimeType, name);
}

AVCodecAudioDecoderImpl::AVCodecAudioDecoderImpl()
{
    MEDIA_LOGD("AVCodecAudioDecoderImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecAudioDecoderImpl::~AVCodecAudioDecoderImpl()
{
    if (codecService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyAVCodecService(codecService_);
        codecService_ = nullptr;
    }
    MEDIA_LOGD("AVCodecAudioDecoderImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecAudioDecoderImpl::Configure(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Configure(format);
}

int32_t AVCodecAudioDecoderImpl::Prepare()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Prepare();
}

int32_t AVCodecAudioDecoderImpl::Start()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Start();
}

int32_t AVCodecAudioDecoderImpl::Stop()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Stop();
}

int32_t AVCodecAudioDecoderImpl::Flush()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Flush();
}

int32_t AVCodecAudioDecoderImpl::Reset()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Reset();
}

int32_t AVCodecAudioDecoderImpl::Release()
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->Release();
}

std::shared_ptr<AVSharedMemory> AVCodecAudioDecoderImpl::GetInputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, nullptr, "service died");
    return codecService_->GetInputBuffer(index);
}

int32_t AVCodecAudioDecoderImpl::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->QueueInputBuffer(index, info, flag);
}

std::shared_ptr<AVSharedMemory> AVCodecAudioDecoderImpl::GetOutputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, nullptr, "service died");
    return codecService_->GetOutputBuffer(index);
}

int32_t AVCodecAudioDecoderImpl::GetOutputFormat(Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->GetOutputFormat(format);
}

int32_t AVCodecAudioDecoderImpl::ReleaseOutputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->ReleaseOutputBuffer(index);
}

int32_t AVCodecAudioDecoderImpl::SetParameter(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    return codecService_->SetParameter(format);
}

int32_t AVCodecAudioDecoderImpl::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, MSERR_INVALID_OPERATION, "service died");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, MSERR_INVALID_VAL, "callback is nullptr");
    return codecService_->SetCallback(callback);
}
} // nmamespace Media
} // namespace OHOS
