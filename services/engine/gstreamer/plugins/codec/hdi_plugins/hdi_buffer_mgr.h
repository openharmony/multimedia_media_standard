/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef HDI_BUFFER_MGR_H
#define HDI_BUFFER_MGR_H

#include <mutex>
#include <list>
#include <vector>
#include <condition_variable>
#include <gst/gst.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include "nocopyable.h"
#include "i_codec_buffer_mgr.h"
#include "codec_component_type.h"
#include "codec_component_if.h"
#include "buffer_type_meta.h"
#include "codec_omx_ext.h"

namespace OHOS {
namespace Media {
struct HdiBufferWrap {
    intptr_t buf;
    OmxCodecBuffer hdiBuffer;
};
class HdiBufferMgr : public NoCopyable, public ICodecBufferMgr {
public:
    HdiBufferMgr();
    virtual ~HdiBufferMgr() override;
    virtual void Init(CodecComponentType *handle, int32_t index, const CompVerInfo &verInfo);
    virtual int32_t Start();

    int32_t AllocateBuffers() override
    {
        return GST_CODEC_OK;
    }

    int32_t UseBuffers(std::vector<GstBuffer *> buffers) override
    {
        (void)buffers;
        return GST_CODEC_OK;
    }

    int32_t PushBuffer(GstBuffer *buffer) override
    {
        (void)buffer;
        return GST_CODEC_OK;
    }

    int32_t PullBuffer(GstBuffer **buffer) override
    {
        (void)buffer;
        return GST_CODEC_OK;
    }

    int32_t FreeBuffers() override
    {
        return GST_CODEC_OK;
    }

    virtual int32_t Preprocessing()
    {
        return GST_CODEC_OK;
    }

    virtual int32_t CodecBufferAvailable(const OmxCodecBuffer *buffer)
    {
        (void)buffer;
        return GST_CODEC_OK;
    }

    virtual int32_t Flush(bool enable) override;
    virtual int32_t Stop();
    virtual void WaitFlushed();

protected:
    void FreeCodecBuffers();
    bool isFlushing_ = false;
    bool isFlushed_ = false;
    bool isStart_ = false;
    int32_t mPortIndex_ = 0;
    CompVerInfo verInfo_ = {};
    std::mutex mutex_;
    std::condition_variable flushCond_;
    std::condition_variable bufferCond_;
    std::condition_variable freeCond_;
    OMX_PARAM_PORTDEFINITIONTYPE mPortDef_ = {};
    CodecComponentType *handle_ = nullptr;
    std::list<std::shared_ptr<HdiBufferWrap>> availableBuffers_;
    std::list<std::pair<std::shared_ptr<HdiBufferWrap>, GstBuffer *>> codingBuffers_;
    std::vector<std::shared_ptr<HdiBufferWrap>> PreUseAshareMems(std::vector<GstBuffer *> &buffers);
    int32_t UseHdiBuffers(std::vector<std::shared_ptr<HdiBufferWrap>> &buffers);
    virtual std::shared_ptr<HdiBufferWrap> GetCodecBuffer(GstBuffer *buffer);
    virtual void UpdateCodecMeta(GstBufferTypeMeta *bufferType, std::shared_ptr<HdiBufferWrap> &codecBuffer);
    void NotifyAvailable();
};
} // namespace Media
} // namespace OHOS
#endif // HDI_BUFFER_MGR_H
