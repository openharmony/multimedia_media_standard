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

#include "avcodec_listener_stub.h"
#include "media_errors.h"
#include "media_log.h"
#include "media_parcel.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecListenerStub"};
}

namespace OHOS {
namespace Media {
AVCodecListenerStub::AVCodecListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecListenerStub::~AVCodecListenerStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int AVCodecListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (AVCodecListenerStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }

    switch (code) {
        case AVCodecListenerMsg::ON_ERROR: {
            int32_t errorType = data.ReadInt32();
            int32_t errorCode = data.ReadInt32();
            OnError(static_cast<AVCodecErrorType>(errorType), errorCode);
            return MSERR_OK;
        }
        case AVCodecListenerMsg::ON_OUTPUT_FORMAT_CHANGED: {
            Format format;
            (void)MediaParcel::Unmarshalling(data, format);
            OnOutputFormatChanged(format);
            return MSERR_OK;
        }
        case AVCodecListenerMsg::ON_INPUT_BUFFER_AVAILABLE: {
            uint32_t index = data.ReadUint32();
            OnInputBufferAvailable(index);
            return MSERR_OK;
        }
        case AVCodecListenerMsg::ON_OUTPUT_BUFFER_AVAILABLE: {
            uint32_t index = data.ReadUint32();
            AVCodecBufferInfo info;
            info.presentationTimeUs = data.ReadInt64();
            info.size = data.ReadInt32();
            info.offset = data.ReadInt32();
            AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(data.ReadInt32());
            OnOutputBufferAvailable(index, info, flag);
            return MSERR_OK;
        }
        default: {
            MEDIA_LOGE("default case, need check AVCodecListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

void AVCodecListenerStub::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (callback_ != nullptr) {
        callback_->OnError(errorType, errorCode);
    }
}

void AVCodecListenerStub::OnOutputFormatChanged(const Format &format)
{
    if (callback_ != nullptr) {
        callback_->OnOutputFormatChanged(format);
    }
}

void AVCodecListenerStub::OnInputBufferAvailable(uint32_t index)
{
    if (callback_ != nullptr) {
        callback_->OnInputBufferAvailable(index);
    }
}

void AVCodecListenerStub::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    if (callback_ != nullptr) {
        callback_->OnOutputBufferAvailable(index, info, flag);
    }
}

void AVCodecListenerStub::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    callback_ = callback;
}
} // namespace Media
} // namespace OHOS
