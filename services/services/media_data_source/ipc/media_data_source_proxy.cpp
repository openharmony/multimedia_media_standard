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

#include "media_data_source_proxy.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemory_ipc.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceProxy"};
}

namespace OHOS {
namespace Media {
MediaDataCallback::MediaDataCallback(const sptr<IStandardMediaDataSource> &ipcProxy)
    : callbackProxy_(ipcProxy)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataCallback::~MediaDataCallback()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

int32_t MediaDataCallback::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    CHECK_AND_RETURN_RET_LOG(callbackProxy_ != nullptr, SOURCE_ERROR_IO, "callbackProxy_ is nullptr");
    return callbackProxy_->ReadAt(length, mem);
}

int32_t MediaDataCallback::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    CHECK_AND_RETURN_RET_LOG(callbackProxy_ != nullptr, SOURCE_ERROR_IO, "callbackProxy_ is nullptr");
    return callbackProxy_->ReadAt(pos, length, mem);
}

int32_t MediaDataCallback::GetSize(int64_t &size)
{
    CHECK_AND_RETURN_RET_LOG(callbackProxy_ != nullptr, MSERR_INVALID_OPERATION, "callbackProxy_ is nullptr");
    return callbackProxy_->GetSize(size);
}

MediaDataSourceProxy::MediaDataSourceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardMediaDataSource>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceProxy::~MediaDataSourceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t MediaDataSourceProxy::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteInt64(pos);
    data.WriteUint32(length);
    CHECK_AND_RETURN_RET_LOG(WriteAVSharedMemoryToParcel(mem, data) == MSERR_OK, 0, "write parcel failed");
    int error = Remote()->SendRequest(ListenerMsg::READ_AT_POS, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("on info failed, error: %{public}d", error);
        return 0;
    }
    int32_t realLen = reply.ReadInt32();
    return realLen;
}

int32_t MediaDataSourceProxy::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    data.WriteUint32(length);
    CHECK_AND_RETURN_RET_LOG(WriteAVSharedMemoryToParcel(mem, data) == MSERR_OK, 0, "write parcel failed");
    int error = Remote()->SendRequest(ListenerMsg::READ_AT, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("on info failed, error: %{public}d", error);
        return 0;
    }
    int32_t realLen = reply.ReadInt32();
    return realLen;
}

int32_t MediaDataSourceProxy::GetSize(int64_t &size)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC);
    int error = Remote()->SendRequest(ListenerMsg::GET_SIZE, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("on info failed, error: %{public}d", error);
        return -1;
    }
    size = reply.ReadInt64();
    int32_t ret = reply.ReadInt32();
    return ret;
}
} // namespace Media
} // namespace OHOS