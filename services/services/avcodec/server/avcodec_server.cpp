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

#include "avcodec_server.h"
#include "media_log.h"
#include "media_errors.h"
#include "engine_factory_repo.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecServer"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<IAVCodecService> AVCodecServer::Create()
{
    std::shared_ptr<AVCodecServer> server = std::make_shared<AVCodecServer>();
    int32_t ret = server->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("failed to init AVCodecServer");
        return nullptr;
    }
    return server;
}

AVCodecServer::AVCodecServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecServer::~AVCodecServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecServer::Init()
{
    auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(IEngineFactory::Scene::SCENE_AVCODEC);
    CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_REC_ENGINE_FAILED, "failed to get factory");
    codecEngine_ = engineFactory->CreateAVCodecEngine();
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_CREATE_REC_ENGINE_FAILED,
        "Failed to create codec engine");
    status_ = AVCODEC_INITIALIZED;
    return MSERR_OK;
}

int32_t AVCodecServer::InitParameter(AVCodecType type, bool isMimeType, const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = codecEngine_->Init(type, isMimeType, name);
    return ret;
}

int32_t AVCodecServer::Configure(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_INITIALIZED, MSERR_INVALID_OPERATION, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = codecEngine_->Configure(format);
    status_ = (ret == MSERR_OK ? AVCODEC_CONFIGURED : AVCODEC_ERROR);
    return ret;
}

int32_t AVCodecServer::Prepare()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_CONFIGURED, MSERR_INVALID_OPERATION, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = codecEngine_->Prepare();
    status_ = (ret == MSERR_OK ? AVCODEC_PREPARED : AVCODEC_ERROR);
    return ret;
}

int32_t AVCodecServer::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_PREPARED, MSERR_INVALID_OPERATION, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = codecEngine_->Start();
    status_ = (ret == MSERR_OK ? AVCODEC_RUNNING : AVCODEC_ERROR);
    return ret;
}

int32_t AVCodecServer::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_RUNNING || status_ == AVCODEC_END_OF_STREAM,
        MSERR_INVALID_OPERATION, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = codecEngine_->Stop();
    status_ = (ret == MSERR_OK ? AVCODEC_PREPARED : AVCODEC_ERROR);
    return ret;
}

int32_t AVCodecServer::Flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_RUNNING || status_ == AVCODEC_END_OF_STREAM,
        MSERR_INVALID_OPERATION, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = codecEngine_->Flush();
    status_ = (ret == MSERR_OK ? AVCODEC_RUNNING : AVCODEC_ERROR);
    return ret;
}

int32_t AVCodecServer::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = codecEngine_->Reset();
    status_ = (ret == MSERR_OK ? AVCODEC_INITIALIZED : AVCODEC_ERROR);
    return ret;
}

int32_t AVCodecServer::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    codecEngine_ = nullptr;
    return MSERR_OK;
}

sptr<Surface> AVCodecServer::CreateInputSurface()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_CONFIGURED, nullptr, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, nullptr, "engine is nullptr");
    return codecEngine_->CreateInputSurface();
}

int32_t AVCodecServer::SetOutputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_CONFIGURED, MSERR_INVALID_OPERATION, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    return codecEngine_->SetOutputSurface(surface);
}

std::shared_ptr<AVSharedMemory> AVCodecServer::GetInputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_RUNNING, nullptr, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, nullptr, "engine is nullptr");
    return codecEngine_->GetInputBuffer(index);
}

int32_t AVCodecServer::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_RUNNING, MSERR_INVALID_OPERATION, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    int32_t ret = codecEngine_->QueueInputBuffer(index, info, flag);
    if (flag & AVCODEC_BUFFER_FLAG_EOS) {
        if (ret == MSERR_OK) {
            status_ = AVCODEC_END_OF_STREAM;
            MEDIA_LOGI("EOS state");
        }
    }
    return ret;
}

std::shared_ptr<AVSharedMemory> AVCodecServer::GetOutputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_RUNNING || status_ == AVCODEC_END_OF_STREAM,
        nullptr, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, nullptr, "engine is nullptr");
    return codecEngine_->GetOutputBuffer(index);
}

int32_t AVCodecServer::GetOutputFormat(Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ != AVCODEC_UNINITIALIZED, MSERR_INVALID_OPERATION, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    return codecEngine_->GetOutputFormat(format);
}

std::shared_ptr<AudioCaps> AVCodecServer::GetAudioCaps()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, nullptr, "engine is nullptr");
    return codecEngine_->GetAudioCaps();
}
std::shared_ptr<VideoCaps> AVCodecServer::GetVideoCaps()
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, nullptr, "engine is nullptr");
    return codecEngine_->GetVideoCaps();
}

int32_t AVCodecServer::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_RUNNING || status_ == AVCODEC_END_OF_STREAM,
        MSERR_INVALID_OPERATION, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    return codecEngine_->ReleaseOutputBuffer(index, render);
}

int32_t AVCodecServer::SetParameter(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ != AVCODEC_INITIALIZED && status_ != AVCODEC_CONFIGURED,
        MSERR_INVALID_OPERATION, "invalid state");
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    return codecEngine_->SetParameter(format);
}

int32_t AVCodecServer::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == AVCODEC_INITIALIZED, MSERR_INVALID_OPERATION, "invalid state");
    {
        std::lock_guard<std::mutex> cbLock(cbMutex_);
        codecCb_ = callback;
    }
    CHECK_AND_RETURN_RET_LOG(codecEngine_ != nullptr, MSERR_NO_MEMORY, "engine is nullptr");
    std::shared_ptr<IAVCodecEngineObs> obs = shared_from_this();
    (void)codecEngine_->SetObs(obs);
    return MSERR_OK;
}

void AVCodecServer::OnError(int32_t errorType, int32_t errorCode)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnError(static_cast<AVCodecErrorType>(errorType), errorCode);
}

void AVCodecServer::OnOutputFormatChanged(const Format &format)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnOutputFormatChanged(format);
}

void AVCodecServer::OnInputBufferAvailable(uint32_t index)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnInputBufferAvailable(index);
}

void AVCodecServer::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnOutputBufferAvailable(index, info, flag);
}
}
}
