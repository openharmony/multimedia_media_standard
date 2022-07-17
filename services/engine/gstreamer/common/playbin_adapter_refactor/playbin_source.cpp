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

#include "playbin_source.h"
#include "gst_utils.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayBinSource"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<SourceBase> SourceBase::Create(const UriHelper &uri)
{
    if (uri.UriType() == UriHelper::URI_TYPE_HTTP || uri.UriType() == UriHelper::URI_TYPE_HTTPS) {
        MEDIA_LOGD("Success to create HttpSource");
        return std::make_shared<NetWorkSource>();
    }

    MEDIA_LOGD("Success to create SourceBase");
    return std::make_shared<SourceBase>();
}

std::shared_ptr<SourceBase> SourceBase::Create(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    return std::make_shared<AVDataSource>(dataSrc);
}

AVDataSource::AVDataSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
    : avDataSrc_(avDataSrc_)
{
}

std::string AVDataSource::GetGstUrlDesc()
{
    return "appsrc://";
}

int32_t AVDataSource::Start()
{
    CHECK_AND_RETURN_RET_LOG(avDataSrc_ != nullptr, MSERR_INVALID_VAL, "input dataSrc is empty!");
    if (appSrc_ == nullptr) {
        appSrc_ = GstAppsrcWrap::Create(avDataSrc_);
        CHECK_AND_RETURN_RET_LOG(appSrc_ != nullptr, MSERR_NO_MEMORY, "new appsrcwrap failed!");
    }

    return appSrc_->Prepare();
}

int32_t AVDataSource::Stop()
{
    if (appSrc_ == nullptr) {
        return MSERR_INVALID_OPERATION;
    }

    appSrc_->Stop();
    return MSERR_OK;
}

bool AVDataSource::IsSeekable()
{
    if (appSrc_ == nullptr) {
        return false;
    }

    return appSrc_->IsLiveMode();
}

void AVDataSource::OnSourceSetup(ElemPtr source)
{
    if (appSrc_ == nullptr) {
        return;
    }

    if (!MatchElementByTypeName(*source, "GstAppSrc")) {
        return;
    }

    appSrc_->SetAppsrc(source);
}

void AVDataSource::SetMsgNotifier(const MsgNotifier &notifier)
{
    if (appSrc_ == nullptr) {
        return;
    }

    notifier_ = notifier;
    auto msgNotifier = std::bind(&AVDataSource::OnAppsrcErrorMessageReceived, this, std::placeholders::_1);
    CHECK_AND_RETURN_LOG(appSrc_->SetErrorCallback(msgNotifier) == MSERR_OK, "set callback failed");
}

void AVDataSource::OnAppsrcErrorMessageReceived(int32_t errorCode)
{
    if (errorCode != MSERR_OK) {
        notifier_(InnerMessage { INNER_MSG_ERROR, errorCode });
    }
}

const uint64_t NetWorkSource::RING_BUFFER_MAX_SIZE = 5242880; // 5 * 1024 * 1024
const int32_t NetWorkSource::PLAYBIN_QUEUE_MAX_SIZE = 100 * 1024 * 1024; // 100 * 1024 * 1024 Bytes
const uint64_t NetWorkSource::BUFFER_DURATION = 15000000000; // 15s
const int32_t NetWorkSource::BUFFER_LOW_PERCENT_DEFAULT = 1;
const int32_t NetWorkSource::BUFFER_HIGH_PERCENT_DEFAULT = 4;
const uint32_t NetWorkSource::HTTP_TIME_OUT_DEFAULT = 15; // 15s

std::string NetWorkSource::GetGstUrlDesc()
{
    return uriHelper_.FormattedUri();
}

void NetWorkSource::OnPlayBinSetup(ElemPtr playbin)
{
    g_object_set(playbin, "ring-buffer-max-size", RING_BUFFER_MAX_SIZE, nullptr);
    g_object_set(playbin, "buffering-flags", true, "buffer-size", PLAYBIN_QUEUE_MAX_SIZE,
        "buffer-duration", BUFFER_DURATION, "low-percent", BUFFER_LOW_PERCENT_DEFAULT,
        "high-percent", BUFFER_HIGH_PERCENT_DEFAULT, nullptr);
    g_object_set(playbin, "timeout", HTTP_TIME_OUT_DEFAULT, nullptr);
}

void NetWorkSource::OnSourceSetup(ElemPtr source)
{
    if (!MatchElementByTypeName(*source, "GstCurlHttpSrc")) {
        return;
    }

    g_object_set(source, "ssl-ca-file", "/etc/ssl/certs/cacert.pem", nullptr);
    MEDIA_LOGI("setup curl_http ca_file done");
}
}
}