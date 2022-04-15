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

#include "avcodeclist_server.h"
#include "media_log.h"
#include "media_errors.h"
#include "engine_factory_repo.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecListServer"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<IAVCodecListService> AVCodecListServer::Create()
{
    std::shared_ptr<AVCodecListServer> server = std::make_shared<AVCodecListServer>();
    int32_t ret = server->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("failed to init AVCodecListServer");
        return nullptr;
    }
    return server;
}

AVCodecListServer::AVCodecListServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecListServer::~AVCodecListServer()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecListServer::Init()
{
    auto engineFactory = EngineFactoryRepo::Instance().GetEngineFactory(IEngineFactory::Scene::SCENE_AVCODECLIST);
    CHECK_AND_RETURN_RET_LOG(engineFactory != nullptr, MSERR_CREATE_REC_ENGINE_FAILED, "failed to get factory");
    codecListEngine_ = engineFactory->CreateAVCodecListEngine();
    CHECK_AND_RETURN_RET_LOG(codecListEngine_ != nullptr, MSERR_CREATE_REC_ENGINE_FAILED,
        "Failed to create codec list engine");
    return MSERR_OK;
}

std::string AVCodecListServer::FindVideoDecoder(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecListEngine_ != nullptr, "", "engine is nullptr");

    return codecListEngine_->FindVideoDecoder(format);
}

std::string AVCodecListServer::FindVideoEncoder(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecListEngine_ != nullptr, "", "engine is nullptr");

    return codecListEngine_->FindVideoEncoder(format);
}

std::string AVCodecListServer::FindAudioDecoder(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecListEngine_ != nullptr, "", "engine is nullptr");

    return codecListEngine_->FindAudioDecoder(format);
}

std::string AVCodecListServer::FindAudioEncoder(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecListEngine_ != nullptr, "", "engine is nullptr");

    return codecListEngine_->FindAudioEncoder(format);
}

std::vector<CapabilityData> AVCodecListServer::GetCodecCapabilityInfos()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return codecListEngine_->GetCodecCapabilityInfos();
}
} // namespace Media
} // namespace OHOS
