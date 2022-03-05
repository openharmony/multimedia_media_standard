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

#include "media_data_source_stub.h"
#include "media_log.h"
#include "media_errors.h"
#include "media_data_source.h"
#include "avsharedmemory_ipc.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaDataSourceStub"};
}

namespace OHOS {
namespace Media {
MediaDataSourceStub::MediaDataSourceStub(const std::shared_ptr<IMediaDataSource> &dataSrc)
    : dataSrc_(dataSrc)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaDataSourceStub::~MediaDataSourceStub()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int MediaDataSourceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (MediaDataSourceStub::GetDescriptor() != remoteDescriptor) {
        MEDIA_LOGE("Invalid descriptor");
        return MSERR_INVALID_OPERATION;
    }

    switch (code) {
        case ListenerMsg::READ_AT: {
            uint32_t length = data.ReadUint32();
            std::shared_ptr<AVSharedMemory> mem = ReadAVSharedMemoryFromParcel(data);
            int32_t realLen = ReadAt(length, mem);
            reply.WriteInt32(realLen);
            return MSERR_OK;
        }
        case ListenerMsg::READ_AT_POS: {
            int64_t pos = data.ReadInt64();
            uint32_t length = data.ReadUint32();
            std::shared_ptr<AVSharedMemory> mem = ReadAVSharedMemoryFromParcel(data);
            int32_t realLen = ReadAt(pos, length, mem);
            reply.WriteInt32(realLen);
            return MSERR_OK;
        }
        case ListenerMsg::GET_SIZE: {
            int64_t size = 0;
            int32_t ret = GetSize(size);
            reply.WriteInt64(size);
            reply.WriteInt32(ret);
            return MSERR_OK;
        }
        default: {
            MEDIA_LOGE("default case, need check MediaDataSourceStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }
}

int32_t MediaDataSourceStub::ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    CHECK_AND_RETURN_RET_LOG(dataSrc_ != nullptr, SOURCE_ERROR_IO, "dataSrc_ is nullptr");
    return dataSrc_->ReadAt(pos, length, mem);
}

int32_t MediaDataSourceStub::ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
{
    CHECK_AND_RETURN_RET_LOG(dataSrc_ != nullptr, SOURCE_ERROR_IO, "dataSrc_ is nullptr");
    return dataSrc_->ReadAt(length, mem);
}

int32_t MediaDataSourceStub::GetSize(int64_t &size)
{
    CHECK_AND_RETURN_RET_LOG(dataSrc_ != nullptr, MSERR_INVALID_OPERATION, "dataSrc_ is nullptr");
    return dataSrc_->GetSize(size);
}
} // namespace Media
} // namespace OHOS