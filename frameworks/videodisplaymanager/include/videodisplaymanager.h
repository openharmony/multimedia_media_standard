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

#ifndef VIDEODISPLAYMANAGER_H
#define VIDEODISPLAYMANAGER_H

#include <refbase.h>
#include <surface.h>

#include "display_type.h"
#include "ibuffer_producer.h"

namespace OHOS {
class Listener : public IBufferConsumerListener {
public:
    Listener(sptr<Surface> impl, uint32_t id)
    {
        surface_ = impl;
        layerId_ = id;
        preBuffer = nullptr;
        timestamp = 0;
    }

    ~Listener();
    void OnBufferAvailable() override;

private:
    wptr<Surface> surface_;
    uint32_t layerId_;
    int64_t timestamp;
    Rect damage;
    sptr<SurfaceBuffer> preBuffer;
};

class VideoDisplayManager : public RefBase  {
public:
    VideoDisplayManager();
    virtual ~VideoDisplayManager();
    static int32_t CreateLayer(const LayerInfo &layerInfo, uint32_t &layerId, sptr<Surface>& surface);
    static void DestroyLayer(uint32_t layerId);
    sptr<IBufferProducer> AttachLayer(sptr<Surface>& surface, uint32_t layerId);
    void DetachLayer(uint32_t layerId);
    int32_t SetRect(uint32_t layerId, IRect rect);
    int32_t GetRect(uint32_t layerId, IRect &rect);
    int32_t SetZorder(uint32_t layerId, uint32_t zorder);
    int32_t GetZorder(uint32_t layerId, uint32_t &zorder);
    int32_t SetTransformMode(uint32_t layerId, TransformType type);
    int32_t SetVisable(uint32_t layerId, bool visible);
    int32_t IsVisable(uint32_t layerId, bool &visible);

private:
    sptr<Surface> surface_;
    sptr<IBufferConsumerListener> listener;
};
} // namespace OHOS
#endif // VIDEODISPLAYMANAGER_H
