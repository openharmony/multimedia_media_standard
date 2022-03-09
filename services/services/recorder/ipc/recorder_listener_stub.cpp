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

#include "recorder_listener_stub.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderListenerStub"};
}

namespace OHOS {
namespace Media {
RecorderListenerStub::RecorderListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

RecorderListenerStub::~RecorderListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int RecorderListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (RecorderListenerStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }

    switch (code) {
        case RecorderListenerMsg::ON_ERROR: {
            int errorType = data.ReadInt32();
            int errorCode = data.ReadInt32();
            OnError(static_cast<RecorderErrorType>(errorType), errorCode);
            return MSERR_OK;
        }
        case RecorderListenerMsg::ON_INFO: {
            int type = data.ReadInt32();
            int extra = data.ReadInt32();
            OnInfo(type, extra);
            return MSERR_OK;
        }
        default: {
            MEDIA_LOGE("default case, need check RecorderListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void RecorderListenerStub::OnError(int32_t errorType, int32_t errorCode)
{
    if (callback_ != nullptr) {
        callback_->OnError(static_cast<RecorderErrorType>(errorType), errorCode);
    }
}

void RecorderListenerStub::OnInfo(int32_t type, int32_t extra)
{
    if (callback_ != nullptr) {
        callback_->OnInfo(type, extra);
    }
}

void RecorderListenerStub::SetRecorderCallback(const std::shared_ptr<RecorderCallback> &callback)
{
    callback_ = callback;
}
} // namespace Media
} // namespace OHOS
