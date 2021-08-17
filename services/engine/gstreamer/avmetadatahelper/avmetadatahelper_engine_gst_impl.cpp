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
#include "media_errors.h"
#include "media_log.h"

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
}

int32_t AVMetadataHelperEngineGstImpl::SetSource(const std::string &uri, int32_t usage)
{
    (void)uri;
    (void)usage;
    return MSERR_OK;
}

std::string AVMetadataHelperEngineGstImpl::ResolveMetadata(int32_t key)
{
    (void)key;
    return "";
}

std::unordered_map<int32_t, std::string> AVMetadataHelperEngineGstImpl::ResolveMetadata()
{
    return {};
}

std::shared_ptr<AVSharedMemory> AVMetadataHelperEngineGstImpl::FetchFrameAtTime(
    int64_t timeUs, int32_t option, OutputConfiguration param)
{
    return AVSharedMemory::Create(100, 0, "placeholder");
}
}
}