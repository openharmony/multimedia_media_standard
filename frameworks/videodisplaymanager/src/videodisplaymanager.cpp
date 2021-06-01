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
#include "buffer_log.h"
#include "display_layer.h"
#include "surface_buffer_impl.h"

static LayerFuncs *g_layerFuncs = nullptr;

#ifndef VIDEO_DISPLAY_DEBUG
#define VIDEO_DISPLAY_ENTER() ((void)0)
#define VIDEO_DISPLAY_EXIT() ((void)0)
#else
#define VIDEO_DISPLAY_ENTER() do { \
    BLOGFD("enter..."); \
} while (0)

#define VIDEO_DISPLAY_EXIT() do { \
    BLOGFD("exit..."); \
} while (0)
#endif

namespace OHOS {
namespace {
    static constexpr HiviewDFX::HiLogLabel LABEL = { LOG_CORE, 0, "VideoDisplayManager" };
}

    Listener::~Listener()
    {
        if (surface_ != nullptr && preBuffer != nullptr) {
            SurfaceError ret = surface_->ReleaseBuffer(preBuffer, -1);
            if (ret != SURFACE_ERROR_OK) {
                BLOGFE("release buffer fail, ret=%{public}d", ret);
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

        if (surface_ == nullptr) {
            BLOGFE("surface is null");
            return;
        }
        ret = surface_->AcquireBuffer(buffer, fence, timestamp, damage);
        if (ret != SURFACE_ERROR_OK) {
            BLOGFE("acquire buffer fail, ret=%{public}d", ret);
            return;
        }
        bufferImpl = SurfaceBufferImpl::FromBase(buffer);
        if (g_layerFuncs != nullptr && g_layerFuncs->SetLayerBuffer != nullptr) {
            g_layerFuncs->SetLayerBuffer(0, layerId_, bufferImpl->GetBufferHandle(), fence);
        }
        if (preBuffer != nullptr) {
            ret = surface_->ReleaseBuffer(preBuffer, -1);
            if (ret != SURFACE_ERROR_OK) {
                BLOGFE("release buffer fail, ret=%{public}d", ret);
            }
        }
        preBuffer = buffer;
    }

    VideoDisplayManager::VideoDisplayManager() : surface_(nullptr), listener(nullptr)
    {
        VIDEO_DISPLAY_ENTER();
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
        DisplayInfo dspInfo = {};
        LayerInfo info = layerInfo;

        if (g_layerFuncs == nullptr) {
            ret = LayerInitialize(&g_layerFuncs);
            if (ret != DISPLAY_SUCCESS || g_layerFuncs == nullptr) {
                BLOGFE("layer init fail, ret=%{public}d",  ret);
                return DISPLAY_FAILURE;
            }
        }

        if (g_layerFuncs->InitDisplay == nullptr || g_layerFuncs->CreateLayer == nullptr ||
            g_layerFuncs->GetDisplayInfo == nullptr) {
            BLOGFE("layer func is invalid");
            return DISPLAY_FAILURE;
        }

        ret = g_layerFuncs->InitDisplay(0);
        if (ret != DISPLAY_SUCCESS) {
            BLOGFE("init display fail, ret=%{public}d",  ret);
            return DISPLAY_FAILURE;
        }

        ret = g_layerFuncs->GetDisplayInfo(0, &dspInfo);
        if (ret != DISPLAY_SUCCESS) {
            BLOGFE("get display info fail, ret=%{public}d",  ret);
            (void)g_layerFuncs->DeinitDisplay(0);
            return DISPLAY_FAILURE;
        }

        if (info.height == 0 && info.width == 0) {
            info.width = dspInfo.width;
            info.height = dspInfo.height;
        }

        BLOGFI("create layer width=%{public}d height=%{public}d type=%{public}d bpp=%{public}d pixFormat=%{public}d",
            info.width, info.height, info.type, info.bpp, info.pixFormat);

        ret = g_layerFuncs->CreateLayer(0, &info, &layerId);
        if (ret != DISPLAY_SUCCESS) {
            BLOGFE("create layer fail, ret=%{public}d",  ret);
            (void)g_layerFuncs->DeinitDisplay(0);
            return DISPLAY_FAILURE;
        }

        surface = Surface::CreateSurfaceAsConsumer();
        if (surface == nullptr) {
            BLOGFE("create surface fail");
            (void)g_layerFuncs->CloseLayer(0, layerId);
            (void)g_layerFuncs->DeinitDisplay(0);
            return DISPLAY_FAILURE;
        }

        VIDEO_DISPLAY_EXIT();
        return DISPLAY_SUCCESS;
    }

    void VideoDisplayManager::DestroyLayer(uint32_t layerId)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerFuncs != nullptr) {
            if (g_layerFuncs->CloseLayer != nullptr) {
                (void)g_layerFuncs->CloseLayer(0, layerId);
            }
            if (g_layerFuncs->DeinitDisplay != nullptr) {
                (void)g_layerFuncs->DeinitDisplay(0);
            }
        } else {
            BLOGFE("layer is not create");
        }
        VIDEO_DISPLAY_EXIT();
    }

    sptr<IBufferProducer> VideoDisplayManager::AttachLayer(sptr<Surface>& surface, uint32_t layerId)
    {
        VIDEO_DISPLAY_ENTER();
        surface_ = surface;
        listener = new Listener(surface_, layerId);
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
        if (g_layerFuncs == nullptr || g_layerFuncs->SetLayerSize == nullptr) {
            BLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerFuncs->SetLayerSize(0, layerId, &rect);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::GetRect(uint32_t layerId, IRect &rect)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerFuncs == nullptr || g_layerFuncs->GetLayerSize == nullptr) {
            BLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerFuncs->GetLayerSize(0, layerId, &rect);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::SetZorder(uint32_t layerId, uint32_t zorder)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerFuncs == nullptr || g_layerFuncs->SetLayerZorder == nullptr) {
            BLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerFuncs->SetLayerZorder(0, layerId, zorder);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::GetZorder(uint32_t layerId, uint32_t &zorder)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerFuncs == nullptr || g_layerFuncs->GetLayerZorder == nullptr) {
            BLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerFuncs->GetLayerZorder(0, layerId, &zorder);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::SetTransformMode(uint32_t layerId, TransformType type)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerFuncs == nullptr || g_layerFuncs->SetTransformMode == nullptr) {
            BLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerFuncs->SetTransformMode(0, layerId, type);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::SetVisable(uint32_t layerId, bool visible)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerFuncs == nullptr || g_layerFuncs->SetLayerVisible == nullptr) {
            BLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerFuncs->SetLayerVisible(0, layerId, visible);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }

    int32_t VideoDisplayManager::IsVisable(uint32_t layerId, bool &visible)
    {
        VIDEO_DISPLAY_ENTER();
        if (g_layerFuncs == nullptr || g_layerFuncs->GetLayerVisibleState == nullptr) {
            BLOGFE("layer is not create or invalid");
            return DISPLAY_FAILURE;
        }

        int32_t ret = g_layerFuncs->GetLayerVisibleState(0, layerId, &visible);
        VIDEO_DISPLAY_EXIT();
        return ret;
    }
} // namespace OHOS
