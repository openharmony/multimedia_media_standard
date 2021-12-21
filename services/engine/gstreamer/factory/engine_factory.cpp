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

#include "i_engine_factory.h"
#include "nocopyable.h"
#include "media_errors.h"
#include "media_log.h"
#include "player_engine_gst_impl.h"
#include "recorder_engine_gst_impl.h"
#include "avmetadatahelper_engine_gst_impl.h"
#include "avcodec_engine_gst_impl.h"
#include "avcodeclist_engine_gst_impl.h"
#include "gst_loader.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstEngineFactory"};
}

namespace OHOS {
namespace Media {
class GstEngineFactory : public IEngineFactory {
public:
    GstEngineFactory() = default;
    ~GstEngineFactory() = default;

    int32_t Score(Scene scene, const std::string &uri) override;
    std::unique_ptr<IPlayerEngine> CreatePlayerEngine() override;
    std::unique_ptr<IRecorderEngine> CreateRecorderEngine() override;
    std::unique_ptr<IAVMetadataHelperEngine> CreateAVMetadataHelperEngine() override;
    std::unique_ptr<IAVCodecEngine> CreateAVCodecEngine() override;
    std::unique_ptr<IAVCodecListEngine> CreateAVCodecListEngine() override;

    DISALLOW_COPY_AND_MOVE(GstEngineFactory);
};

int32_t GstEngineFactory::Score(Scene scene, const std::string &uri)
{
    (void)uri;
    (void)scene;
    return MIN_SCORE + 1;
}

std::unique_ptr<IPlayerEngine> GstEngineFactory::CreatePlayerEngine()
{
    GstLoader::Instance().UpdateLogLevel();
    return std::make_unique<PlayerEngineGstImpl>();
}

std::unique_ptr<IAVMetadataHelperEngine> GstEngineFactory::CreateAVMetadataHelperEngine()
{
    GstLoader::Instance().UpdateLogLevel();
    return std::make_unique<AVMetadataHelperEngineGstImpl>();
}

std::unique_ptr<IRecorderEngine> GstEngineFactory::CreateRecorderEngine()
{
    GstLoader::Instance().UpdateLogLevel();
    auto engine = std::make_unique<RecorderEngineGstImpl>();
    int32_t ret = engine->Init();
    if (ret != MSERR_OK) {
        MEDIA_LOGE("recorder engine init failed, ret = %{public}d", ret);
        return nullptr;
    }
    return engine;
}

std::unique_ptr<IAVCodecEngine> GstEngineFactory::CreateAVCodecEngine()
{
    GstLoader::Instance().UpdateLogLevel();
    return std::make_unique<AVCodecEngineGstImpl>();
}

std::unique_ptr<IAVCodecListEngine> GstEngineFactory::CreateAVCodecListEngine()
{
    GstLoader::Instance().UpdateLogLevel();
    return std::make_unique<AVCodecListEngineGstImpl>();
}
}
}

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((visibility("default"))) OHOS::Media::IEngineFactory *CreateEngineFactory()
{
    int32_t ret = OHOS::Media::GstLoader::Instance().SetUp();
    if (ret != OHOS::Media::MSERR_OK) {
        MEDIA_LOGE("Gst Engine setup failed, ret = %{public}d", ret);
        return nullptr;
    }
    return new (std::nothrow) OHOS::Media::GstEngineFactory();
}

#ifdef __cplusplus
}
#endif