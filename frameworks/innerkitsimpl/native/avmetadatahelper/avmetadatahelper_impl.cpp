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

#include "avmetadatahelper_impl.h"
#include "securec.h"
#include "i_media_service.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetadatahelperImpl"};
}

namespace OHOS {
namespace Media {
struct AVShMemWrapper {
    std::shared_ptr<AVSharedMemory> mem;
};

static void FreePixelMapData(void *addr, void *context, uint32_t size)
{
    (void)addr;
    (void)size;

    MEDIA_LOGI("free pixel map data");

    CHECK_AND_RETURN_LOG(context != nullptr, "context is nullptr");
    AVShMemWrapper *wrapper = reinterpret_cast<AVShMemWrapper *>(context);
    CHECK_AND_RETURN_LOG(wrapper->mem != nullptr, "mem is nullptr");
    wrapper->mem = nullptr;
    delete wrapper;
}

static std::shared_ptr<PixelMap> CreatePixelMap(const std::shared_ptr<AVSharedMemory> &mem, PixelFormat color)
{
    OutputFrame *frame = reinterpret_cast<OutputFrame *>(mem->GetBase());
    MEDIA_LOGD("width: %{public}d, height: %{public}d, size: %{public}d, format: %{public}d",
        frame->width_, frame->height_, frame->size_, color);

    ImageInfo info;
    info.size.width = frame->width_;
    info.size.height = frame->height_;
    info.pixelFormat = color;
    info.colorSpace = ColorSpace::SRGB;

    std::shared_ptr<PixelMap> pixelMap = std::make_shared<PixelMap>();
    int32_t ret = pixelMap->SetImageInfo(info);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "Set image info failed");
    CHECK_AND_RETURN_RET_LOG(pixelMap->GetByteCount() == frame->size_, nullptr, "Size inconsistent !");

    AVShMemWrapper *wrapper = new (std::nothrow) AVShMemWrapper;
    CHECK_AND_RETURN_RET_LOG(wrapper != nullptr, nullptr, "alloc avshmem wrapper failed");
    wrapper->mem = mem;

    pixelMap->SetPixelsAddr(frame->GetFlattenedData(), wrapper, static_cast<uint32_t>(pixelMap->GetByteCount()),
        AllocatorType::CUSTOM_ALLOC, FreePixelMapData);
    return pixelMap;
}

std::shared_ptr<AVMetadataHelper> AVMetadataHelperFactory::CreateAVMetadataHelper()
{
    std::shared_ptr<AVMetadataHelperImpl> impl = std::make_shared<AVMetadataHelperImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new AVMetadataHelperImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init AVMetadataHelperImpl");

    return impl;
}

int32_t AVMetadataHelperImpl::Init()
{
    avMetadataHelperService_ = MeidaServiceFactory::GetInstance().CreateAVMetadataHelperService();
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, MSERR_NO_MEMORY,
        "failed to create avmetadatahelper service");
    return MSERR_OK;
}

AVMetadataHelperImpl::AVMetadataHelperImpl()
{
    MEDIA_LOGD("AVMetadataHelperImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMetadataHelperImpl::~AVMetadataHelperImpl()
{
    if (avMetadataHelperService_ != nullptr) {
        (void)MeidaServiceFactory::GetInstance().DestroyAVMetadataHelperService(avMetadataHelperService_);
        avMetadataHelperService_ = nullptr;
    }
    MEDIA_LOGD("AVMetadataHelperImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVMetadataHelperImpl::SetSource(const std::string &uri, int32_t usage)
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, MSERR_NO_MEMORY,
        "avmetadatahelper service does not exist..");
    CHECK_AND_RETURN_RET_LOG(!uri.empty(), MSERR_INVALID_VAL, "uri is empty.");

    return avMetadataHelperService_->SetSource(uri, usage);
}

std::string AVMetadataHelperImpl::ResolveMetadata(int32_t key)
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, "",
        "avmetadatahelper service does not exist.");

    return avMetadataHelperService_->ResolveMetadata(key);
}

std::unordered_map<int32_t, std::string> AVMetadataHelperImpl::ResolveMetadata()
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, {},
        "avmetadatahelper service does not exist.");

    return avMetadataHelperService_->ResolveMetadata();
}

std::shared_ptr<PixelMap> AVMetadataHelperImpl::FetchFrameAtTime(int64_t timeUs, int32_t option, PixelMapParams param)
{
    CHECK_AND_RETURN_RET_LOG(avMetadataHelperService_ != nullptr, nullptr,
        "avmetadatahelper service does not exist.");

    OutputConfiguration config;
    config.colorFormat = param.colorFormat;
    config.dstHeight = param.dstHeight;
    config.dstWidth = param.dstWidth;

    auto mem = avMetadataHelperService_->FetchFrameAtTime(timeUs, option, config);
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, nullptr, "Fetch frame failed");
    CHECK_AND_RETURN_RET_LOG(mem->GetBase() != nullptr, nullptr, "Addr is nullptr");

    if (mem->GetSize() > 0 && static_cast<uint32_t>(mem->GetSize()) >= sizeof(OutputFrame)) {
        return CreatePixelMap(mem, param.colorFormat);
    }

    MEDIA_LOGE("size is incorrect");
    return nullptr;
}

void AVMetadataHelperImpl::Release()
{
    CHECK_AND_RETURN_LOG(avMetadataHelperService_ != nullptr, "avmetadatahelper service does not exist.");
    avMetadataHelperService_->Release();
    (void)MeidaServiceFactory::GetInstance().DestroyAVMetadataHelperService(avMetadataHelperService_);
    avMetadataHelperService_ = nullptr;
}
} // nmamespace Media
} // namespace OHOS
