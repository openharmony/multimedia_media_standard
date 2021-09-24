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

#include "recorder_listener_proxy.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderListenerProxy"};
}

namespace OHOS {
namespace Media {
RecorderListenerProxy::RecorderListenerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardRecorderListener>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderListenerProxy::~RecorderListenerProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void RecorderListenerProxy::OnError(int32_t errorType, int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteInt32(errorType);
    data.WriteInt32(errorCode);
    int error = Remote()->SendRequest(RecorderListenerMsg::ON_ERROR, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("on error failed, error: %{public}d", error);
    }
}

void RecorderListenerProxy::OnInfo(int32_t type, int32_t extra)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    data.WriteInt32(static_cast<int>(type));
    data.WriteInt32(static_cast<int>(extra));
    int error = Remote()->SendRequest(RecorderListenerMsg::ON_INFO, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("on info failed, error: %{public}d", error);
    }
}

RecorderListenerCallback::RecorderListenerCallback(const sptr<IStandardRecorderListener> &listener)
    : listener_(listener)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderListenerCallback::~RecorderListenerCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void RecorderListenerCallback::OnError(RecorderErrorType errorType, int32_t errorCode)
{
    if (listener_ != nullptr) {
        listener_->OnError(errorType, errorCode);
    }
}

void RecorderListenerCallback::OnInfo(int32_t type, int32_t extra)
{
    if (listener_ != nullptr) {
        listener_->OnInfo(type, extra);
    }
}
} // namespace Media
} // namespace OHOS
