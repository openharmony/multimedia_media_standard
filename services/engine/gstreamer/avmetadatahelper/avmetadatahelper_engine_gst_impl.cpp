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

#include "avmetadatahelper_engine_gst_impl.h"
#include <gst/gst.h>
#include "media_errors.h"
#include "media_log.h"
#include "i_playbin_ctrler.h"
#include "avmeta_sinkprovider.h"
#include "frame_converter.h"
#include "scope_guard.h"
#include "uri_helper.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetaEngineGstImpl"};
}

namespace OHOS {
namespace Media {
AVMetadataHelperEngineGstImpl::AVMetadataHelperEngineGstImpl()
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

AVMetadataHelperEngineGstImpl::~AVMetadataHelperEngineGstImpl()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    Reset();
}

int32_t AVMetadataHelperEngineGstImpl::SetSource(const std::string &uri, int32_t usage)
{
    if ((usage != AVMetadataUsage::AV_META_USAGE_META_ONLY) &&
        (usage != AVMetadataUsage::AV_META_USAGE_PIXEL_MAP)) {
        MEDIA_LOGE("Invalid avmetadatahelper usage: %{public}d", usage);
        return MSERR_INVALID_VAL;
    }

    if (UriHelper(uri).FormatMe().UriType() != UriHelper::URI_TYPE_FILE) {
        MEDIA_LOGE("Unsupported uri type : %{public}s", uri.c_str());
        return MSERR_UNSUPPORT;
    }

    MEDIA_LOGI("uri: %{public}s, usage: %{public}d", uri.c_str(), usage);

    int32_t ret = SetSourceInternel(uri, usage);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    return MSERR_OK;
}

std::string AVMetadataHelperEngineGstImpl::ResolveMetadata(int32_t key)
{
    MEDIA_LOGD("enter");
    std::string result;

    int32_t ret = ExtractMetadata();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, result);

    if (collectedMeta_.count(key) == 0) {
        MEDIA_LOGE("The specified metadata %{public}d cannot be obtained from the specified stream.", key);
        return result;
    }

    MEDIA_LOGD("exit");
    result = collectedMeta_[key];
    return result;
}

std::unordered_map<int32_t, std::string> AVMetadataHelperEngineGstImpl::ResolveMetadata()
{
    MEDIA_LOGD("enter");

    int32_t ret = ExtractMetadata();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, {});

    MEDIA_LOGD("exit");
    return collectedMeta_;
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperEngineGstImpl::FetchFrameAtTime(
    int64_t timeUs, int32_t option, OutputConfiguration param)
{
    MEDIA_LOGD("enter");

    if ((option != AV_META_QUERY_CLOSEST) && (option != AV_META_QUERY_CLOSEST_SYNC) &&
        (option != AV_META_QUERY_NEXT_SYNC) && (option != AV_META_QUERY_PREVIOUS_SYNC)) {
        MEDIA_LOGE("Invalid query option: %{public}d", option);
        return nullptr;
    }

    if (usage_ != AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) {
        MEDIA_LOGE("current instance is unavaiable for pixel map, check usage !");
        return nullptr;
    }

    int32_t ret = ExtractMetadata();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, nullptr);

    if (collectedMeta_.count(AV_KEY_HAS_VIDEO) == 0) {
        MEDIA_LOGE("the associated media source does not have video track");
        return nullptr;
    }

    ret = InitConverter(param);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, nullptr);

    ret = PrepareInternel(IPlayBinCtrler::PlayBinScene::THUBNAIL);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, nullptr);

    ret = SeekInternel(timeUs, option);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, nullptr);

    std::shared_ptr<AVSharedMemory> frame = converter_->GetOneFrame(); // need exception awaken up.
    CHECK_AND_RETURN_RET(frame != nullptr, nullptr);

    MEDIA_LOGD("exit");
    return frame;
}

int32_t AVMetadataHelperEngineGstImpl::SetSourceInternel(const std::string &uri, int32_t usage)
{
    Reset();

    auto notifier = std::bind(&AVMetadataHelperEngineGstImpl::OnNotifyMessage, this, std::placeholders::_1);
    playBinCtrler_ = IPlayBinCtrler::Create(IPlayBinCtrler::PlayBinKind::PLAYBIN_KIND_PLAYBIN2, notifier);
    CHECK_AND_RETURN_RET(playBinCtrler_ != nullptr, MSERR_UNKNOWN);

    metaCollector_ = std::make_unique<AVMetaMetaCollector>();
    auto listener = std::bind(&AVMetadataHelperEngineGstImpl::OnNotifyElemSetup, this, std::placeholders::_1);
    playBinCtrler_->SetElemSetupListener(listener);

    auto sinkProvider = std::make_shared<AVMetaSinkProvider>(usage);
    int32_t ret = playBinCtrler_->SetSinkProvider(sinkProvider);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    if (usage == AVMetadataUsage::AV_META_USAGE_PIXEL_MAP) {
        converter_ = std::make_shared<FrameConverter>();
        sinkProvider->SetFrameCallback(converter_);
    }
    sinkProvider_ = sinkProvider;

    ret = playBinCtrler_->SetSource(uri);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    metaCollector_->Start();
    usage_ = usage;

    return MSERR_OK;
}

int32_t AVMetadataHelperEngineGstImpl::InitConverter(const OutputConfiguration &config)
{
    // need to skip the same config
    int32_t ret = converter_->Init(config);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_INVALID_OPERATION);

    ret = converter_->StartConvert();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, MSERR_INVALID_OPERATION);

    return MSERR_OK;
}

int32_t AVMetadataHelperEngineGstImpl::PrepareInternel(IPlayBinCtrler::PlayBinScene scene)
{
    CHECK_AND_RETURN_RET_LOG(playBinCtrler_ != nullptr, MSERR_INVALID_OPERATION, "set source firstly");

    int32_t ret = playBinCtrler_->SetScene(scene);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "set scene failed");

    std::unique_lock<std::mutex> lock(mutex_);
    if (prepared_) {
        return MSERR_OK;
    }

    if (scene == IPlayBinCtrler::PlayBinScene::METADATA) {
        ret = playBinCtrler_->PrepareAsync();
    } else {
        metaCollector_->Stop();
        ret = playBinCtrler_->Prepare();
    }
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "prepare failed");

    return MSERR_OK;
}

int32_t AVMetadataHelperEngineGstImpl::SeekInternel(int64_t timeUs, int32_t option)
{
    std::unique_lock<std::mutex> lock(mutex_);

    int32_t ret = playBinCtrler_->Seek(timeUs, option);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    seeking_ = true;
    cond_.wait(lock, [this]() { return canceled_ || !seeking_; });
    CHECK_AND_RETURN_RET_LOG(!canceled_, MSERR_UNKNOWN, "Canceled !");

    return MSERR_OK;
}

int32_t AVMetadataHelperEngineGstImpl::ExtractMetadata()
{
    if (!hasCollecteMeta_) {
        int32_t ret = PrepareInternel(IPlayBinCtrler::PlayBinScene::METADATA);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

        collectedMeta_ = metaCollector_->GetMetadata();
        hasCollecteMeta_ = true;
    }
    return MSERR_OK;
}

void AVMetadataHelperEngineGstImpl::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (metaCollector_ != nullptr) {
        metaCollector_ = nullptr;
        hasCollecteMeta_ = false;
    }

    if (playBinCtrler_ != nullptr) {
        playBinCtrler_ = nullptr;
        sinkProvider_ = nullptr;
    }

    if (converter_ != nullptr) {
        (void)converter_->StopConvert();
        converter_ = nullptr;
    }

    canceled_ = true;
    seeking_ = false;
    prepared_ = false;
    cond_.notify_all();
}

void AVMetadataHelperEngineGstImpl::OnNotifyMessage(const PlayBinMessage &msg)
{
    switch (msg.type) {
        case PLAYBIN_MSG_STATE_CHANGE: {
            std::unique_lock<std::mutex> lock(mutex_);
            if (msg.code == PLAYBIN_STATE_PREPARED) {
                prepared_ = true;
            }
            break;
        }
        case PLAYBIN_MSG_SEEKDONE: {
            std::unique_lock<std::mutex> lock(mutex_);
            seeking_ = false;
            cond_.notify_one();
            break;
        }
        default:
            break;
    }
}

void AVMetadataHelperEngineGstImpl::OnNotifyElemSetup(GstElement &elem)
{
    std::unique_lock<std::mutex> lock(mutex_);
    metaCollector_->AddMetaSource(elem);
}
}
}
