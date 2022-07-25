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

#ifndef SRC_BASE_H
#define SRC_BASE_H

#include <cstdint>
#include <gst/gst.h>
#include "avcodec_common.h"
#include "avsharedmemory.h"
#include "codec_common.h"
#include "format.h"
#include "i_avcodec_engine.h"
#include "media_errors.h"
#include "surface.h"

namespace OHOS {
namespace Media {
enum SrcType : int32_t {
    SRC_TYPE_BYTEBUFFER = 0,
    SRC_TYPE_SURFACE,
};

class SrcBase {
public:
    virtual ~SrcBase() = default;

    virtual int32_t Init() = 0;
    virtual int32_t Configure(const std::shared_ptr<ProcessorConfig> &config) = 0;

    virtual sptr<Surface> CreateInputSurface(const std::shared_ptr<ProcessorConfig> &inputConfig)
    {
        (void)inputConfig;
        return nullptr;
    }

    virtual int32_t Start()
    {
        return MSERR_OK;
    }

    virtual int32_t Stop()
    {
        return MSERR_OK;
    }

    virtual int32_t Flush()
    {
        return MSERR_OK;
    }

    virtual int32_t NotifyEos()
    {
        return MSERR_INVALID_OPERATION;
    }

    virtual bool Needflush()
    {
        return true;
    }

    virtual const GstElement *GetElement()
    {
        return src_;
    }

    virtual std::shared_ptr<AVSharedMemory> GetInputBuffer(uint32_t index)
    {
        (void)index;
        return nullptr;
    }

    virtual int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
    {
        (void)index;
        (void)info;
        (void)flag;
        return MSERR_INVALID_OPERATION;
    }

    virtual int32_t SetParameter(const Format &format)
    {
        (void)format;
        return MSERR_OK;
    }

    virtual int32_t SetCallback(const std::weak_ptr<IAVCodecEngineObs> &obs)
    {
        (void)obs;
        return MSERR_OK;
    }

protected:
    GstElement *src_ = nullptr;
};
} // namespace Media
} // namespace OHOS
#endif // SRC_BASE_H