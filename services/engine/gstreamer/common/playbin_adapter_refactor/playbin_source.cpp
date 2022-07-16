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
std::unique_ptr<PlayBinSource> PlayBinSource::Create(const UriHelper &uri)
{
    if (uri.UriType() == UriHelper::URI_TYPE_HTTPS) {
        MEDIA_LOGD("Success to create HttpsSource");
        return std::make_unique<HttpsSource>();
    }

    MEDIA_LOGD("Success to create PlayBinSource");
    return std::make_unique<PlayBinSource>();
}

std::unique_ptr<PlayBinSource> PlayBinSource::Create(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    return std::make_unique<AVDataSource>(dataSrc);
}

AVDataSource::AVDataSource(const std::shared_ptr<IMediaDataSource> &dataSrc)
    : avDataSrc_(avDataSrc_)
{
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
    if (appSrc_ == nullptr || source == nullptr) {
        return;
    }

    if (!MatchElementByMeta(*source, GST_ELEMENT_METADATA_LONGNAME, { "AppSrc" })) {
        return;
    }

    appSrc_->SetAppsrc(source);
}

void AVDataSource::SetMsgNotifier(const MsgNotifier &notifier)
{
    if (appSrc_ == nullptr) {
        return;
    }

    if (notifier == nullptr || notifier_ != nullptr) {
        return;
    }

    notifier_ = notifier;
    auto msgNotifier = std::bind(&AVDataSource::OnAppsrcErrorMessageReceived, this, std::placeholders::_1);
    CHECK_AND_RETURN_LOG(appSrc_->SetErrorCallback(msgNotifier) == MSERR_OK, "set callback failed");
}

void AVDataSource::OnAppsrcErrorMessageReceived(int32_t errorCode)
{
    notifier_(errorCode);
}

void HttpsSource::OnSourceSetup(ElemPtr source)
{
    if (source == nullptr) {
        return;
    }

    if (!MatchElementByMeta(*source, GST_ELEMENT_METADATA_LONGNAME, { "HTTP Client Source using libcURL" })) {
        return;
    }

    g_object_set(source, "ssl-ca-file", "/etc/ssl/certs/cacert.pem", nullptr);
    MEDIA_LOGI("setup curl_http ca_file done");
}
}
}