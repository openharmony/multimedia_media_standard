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

#include "avmetadatahelper_service_proxy.h"
#include "media_log.h"
#include "media_errors.h"
#include "avsharedmemory_ipc.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetadataHelperServiceProxy"};
}

namespace OHOS {
namespace Media {
AVMetadataHelperServiceProxy::AVMetadataHelperServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardAVMetadataHelperService>(impl)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMetadataHelperServiceProxy::~AVMetadataHelperServiceProxy()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destory", FAKE_POINTER(this));
}

int32_t AVMetadataHelperServiceProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(DESTROY, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("destroy failed, error: %{public}d", error);
        return error;
    }
    return reply.ReadInt32();
}

int32_t AVMetadataHelperServiceProxy::SetSource(const std::string &uri, int32_t usage)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    (void)data.WriteString(uri);
    (void)data.WriteInt32(usage);

    int error = Remote()->SendRequest(SET_SOURCE, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("Set Source failed, error: %{public}d", error);
        return error;
    }
    return reply.ReadInt32();
}

std::string AVMetadataHelperServiceProxy::ResolveMetadata(int32_t key)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    (void)data.WriteInt32(key);

    int error = Remote()->SendRequest(RESOLVE_METADATA, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("ResolveMetadata failed, error: %{public}d", error);
        return "";
    }
    return reply.ReadString();
}

std::unordered_map<int32_t, std::string> AVMetadataHelperServiceProxy::ResolveMetadataMap()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    std::unordered_map<int32_t, std::string> metadata;
    int error = Remote()->SendRequest(RESOLVE_METADATA_MAP, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("ResolveMetadata failed, error: %{public}d", error);
        return metadata;
    }

    std::vector<int32_t> key;
    (void)reply.ReadInt32Vector(&key);
    if (key.size() == 0) {
        MEDIA_LOGE("Read key failed");
        return metadata;
    }

    std::vector<std::string> dataStr;
    (void)reply.ReadStringVector(&dataStr);
    if (dataStr.size() == 0) {
        MEDIA_LOGE("Read dataStr failed");
        return metadata;
    }

    auto itKey = key.begin();
    auto itDataStr = dataStr.begin();
    for (; itKey != key.end() && itDataStr != dataStr.end(); ++itKey, ++itDataStr) {
        metadata[*itKey] = *itDataStr;
    }

    return metadata;
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperServiceProxy::FetchFrameAtTime(int64_t timeUs,
    int32_t option, const OutputConfiguration &param)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption opt;
    (void)data.WriteInt64(timeUs);
    (void)data.WriteInt32(option);
    (void)data.WriteInt32(param.dstWidth);
    (void)data.WriteInt32(param.dstHeight);
    (void)data.WriteInt32(static_cast<int32_t>(param.colorFormat));

    int error = Remote()->SendRequest(FETCH_FRAME_AT_TIME, data, reply, opt);
    if (error != MSERR_OK) {
        MEDIA_LOGE("FetchFrameAtTime failed, error: %{public}d", error);
        return nullptr;
    }
    return ReadAVSharedMemoryFromParcel(reply);
}

void AVMetadataHelperServiceProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    int error = Remote()->SendRequest(RELEASE, data, reply, option);
    if (error != MSERR_OK) {
        MEDIA_LOGE("Release failed, error: %{public}d", error);
    }
}
} // namespace Media
} // namespace OHOS

