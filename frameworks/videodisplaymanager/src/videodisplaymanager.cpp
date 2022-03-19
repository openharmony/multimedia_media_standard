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

#include "videodisplaymanager.h"
#include <hilog/log.h>
#include <iremote_proxy.h>
#include "surface_buffer_impl.h"
#include "idisplay_layer.h"

using namespace OHOS::HDI::Display::V1_0;
static OHOS::sptr<IDisplayLayer> g_layerService = nullptr;
constexpr const char *DISPLAY_LAYER_SERVICE_NAME = "hdi_video_layer_service";

namespace {
    static constexpr ::OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, 0, "VideoDisplayManager" };
}

#define __VLOG(func, fmt, ...) \
    func(LABEL, "%{public}s: " fmt , __func__, ##__VA_ARGS__)

#define VLOGFD(fmt, ...) __VLOG(::OHOS::HiviewDFX::HiLog::Debug, fmt, ##__VA_ARGS__)
#define VLOGFI(fmt, ...) __VLOG(::OHOS::HiviewDFX::HiLog::Info, fmt, ##__VA_ARGS__)
#define VLOGFW(fmt, ...) __VLOG(::OHOS::HiviewDFX::HiLog::Warn, fmt, ##__VA_ARGS__)
#define VLOGFE(fmt, ...) __VLOG(::OHOS::HiviewDFX::HiLog::Error, fmt, ##__VA_ARGS__)

#ifndef VIDEO_DISPLAY_DEBUG
#define VIDEO_DISPLAY_ENTER() ((void)0)
#define VIDEO_DISPLAY_EXIT() ((void)0)
#else
#define VIDEO_DISPLAY_ENTER() do { \
    VLOGFD("enter..."); \
} while (0)

#define VIDEO_DISPLAY_EXIT() do { \
    VLOGFD("exit..."); \
} while (0)
#endif

namespace OHOS {
    Listener::~Listener()
    {
        if (surface_ != nullptr && preBuffer != nullptr) {
            SurfaceError ret = surface_->ReleaseBuffer(preBuffer, -1);
            if (ret != SURFACE_ERROR_OK) {
                VLOGFE("release buffer fail, ret=%{public}d", ret);
            }
            preBuffer = nullptr;
        }
    }

    void Listener::OnBufferAvailable()
    {
        sptr<SurfaceBuffer> buffer;
        sptr<SurfaceBufferImpl> bufferImpl;
        int32_t fence;
        SurfaceError ret;

        VIDEO_DISPLAY_ENTER();
        auto surfacTemp = surface_.promote();
        if (surfacTemp == nullptr) {
            VLOGFE("surface is null");
            return;
        }
        ret = surfacTemp->AcquireBuffer(buffer, fence, timestamp, damage);
        if (ret != SURFACE_ERROR_OK) {
            VLOGFE("acquire buffer fail, ret=%{public}d", ret);
            return;
        }
        bufferImpl = SurfaceBufferImpl::FromBase(buffer);
        if (bufferImpl == nullptr) {
            VLOGFE("bufferImpl is null");
            return;
        }
        if (g_layerService != nullptr) {
            auto bufferHandle = bufferImpl->GetBufferHandle();
            g_layerService->SetLayerBuffer(0, layerId_, *bufferHandle, fence);
        }
        if (preBuffer != nullptr) {
            ret = surfacTemp->ReleaseBuffer(preBuffer, -1);
            if (ret != SURFACE_ERROR_OK) {
                VLOGFE("release buffer fail, ret=%{public}d", ret);
            }
        }
        preBuffer = buffer;
        VIDEO_DISPLAY_EXIT();
    }

    VideoDisplayManager::VideoDisplayManager() : surface_(nullptr), listener(nullptr)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerService == nullptr) {
            g_layerService = IDisplayLayer::Get(DISPLAY_LAYER_SERVICE_NAME);
            if (g_layerService == nullptr) {
                VLOGFE("VideoDisplayManager : get layer service failed");
            }
        }
        VIDEO_DISPLAY_EXIT();
    }

    VideoDisplayManager::~VideoDisplayManager()
    {
        VIDEO_DISPLAY_ENTER();
        VIDEO_DISPLAY_EXIT();
    }

    int32_t VideoDisplayManager::CreateLayer(const LayerInfo &layerInfo, uint32_t &layerId, sptr<Surface>& surface)
    {
        VIDEO_DISPLAY_ENTER();
        int32_t ret;
        std::shared_ptr<DisplayInfo> dspInfo;
        LayerInfo info = layerInfo;

        if (g_layerService == nullptr) {
            g_layerService = IDisplayLayer::Get(DISPLAY_LAYER_SERVICE_NAME);
            if (g_layerService == nullptr) {
                VLOGFE("VideoDisplayManager : get layer service failed");
                return DISPLAY_FAILURE;
            }
        }

        ret = g_layerService->InitDisplay(0);
        if (ret != DISPLAY_SUCCESS) {
            VLOGFE("init display fail, ret=%{public}d",  ret);
            return DISPLAY_FAILURE;
        }

        ret = g_layerService->GetDisplayInfo(0, dspInfo);
        if (ret != DISPLAY_SUCCESS) {
            VLOGFE("get display info fail, ret=%{public}d",  ret);
            (void)g_layerService->DeinitDisplay(0);
            return DISPLAY_FAILURE;
        }

        if (info.height == 0 && info.width == 0) {
            info.width = dspInfo->width;
            info.height = dspInfo->height;
        }

        VLOGFI("create layer width=%{public}d height=%{public}d type=%{public}d bpp=%{public}d pixFormat=%{public}d",
            info.width, info.height, info.type, info.bpp, info.pixFormat);

        ret = g_layerService->CreateLayer(0, info, layerId);
        if (ret != DISPLAY_SUCCESS) {
            VLOGFE("create layer fail, ret=%{public}d",  ret);
            (void)g_layerService->DeinitDisplay(0);
            return DISPLAY_FAILURE;
        }

        surface = Surface::CreateSurfaceAsConsumer();
        if (surface == nullptr) {
            VLOGFE("create surface fail");
            (void)g_layerService->CloseLayer(0, layerId);
            return DISPLAY_FAILURE;
        }

        VIDEO_DISPLAY_EXIT();
        return DISPLAY_SUCCESS;
    }

    void VideoDisplayManager::DestroyLayer(uint32_t layerId)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerService != nullptr) {
            (void)g_layerService->CloseLayer(0, layerId);
        } else {
            VLOGFE("layer is not create");
        }
        VIDEO_DISPLAY_EXIT();
    }

    sptr<IBufferProducer> VideoDisplayManager::AttachLayer(sptr<Surface>& surface, uint32_t layerId)
    {
        VIDEO_DISPLAY_ENTER();
        surface_ = surface;
        listener = new (std::nothrow) Listener(surface_, layerId);
        if (listener == nullptr) {
            VLOGFE("Failed to create listener");
            return nullptr;
        }

        surface_->RegisterConsumerListener(listener);
        VIDEO_DISPLAY_EXIT();
        return surface_->GetProducer();
    }

    void VideoDisplayManager::DetachLayer(uint32_t layerId)
    {
        VIDEO_DISPLAY_ENTER();
        if (surface_ != nullptr) {
            surface_->UnregisterConsumerListener();
        }
        VIDEO_DISPLAY_EXIT();
    }

    int32_t VideoDisplayManager::SetRect(uint32_t layerId, IRect rect)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerService == nullptr) {
            VLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerService->SetLayerRect(0, layerId, rect);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::GetRect(uint32_t layerId, IRect &rect)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerService == nullptr) {
            VLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        std::shared_ptr<IRect> pRect = std::make_shared<IRect>();
        int32_t ret = g_layerService->GetLayerRect(0, layerId, pRect);
        auto temp = pRect.get();
        rect = *temp;
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::SetZorder(uint32_t layerId, uint32_t zorder)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerService == nullptr) {
            VLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerService->SetLayerZorder(0, layerId, zorder);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::GetZorder(uint32_t layerId, uint32_t &zorder)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerService == nullptr) {
            VLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerService->GetLayerZorder(0, layerId, zorder);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::SetTransformMode(uint32_t layerId, TransformType type)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerService == nullptr) {
            VLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerService->SetTransformMode(0, layerId, type);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::SetVisable(uint32_t layerId, bool visible)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerService == nullptr) {
            VLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerService->SetLayerVisible(0, layerId, visible);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::IsVisable(uint32_t layerId, bool &visible)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerService == nullptr) {
            VLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerService->GetLayerVisibleState(0, layerId, visible);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }
} // namespace OHOS
