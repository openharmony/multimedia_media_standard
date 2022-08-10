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

#ifndef HDI_CODEC_H
#define HDI_CODEC_H

#include <memory>
#include <mutex>
#include <condition_variable>
#include <gst/gst.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include "nocopyable.h"
#include "i_gst_codec.h"
#include "hdi_buffer_mgr.h"
#include "hdi_params_mgr.h"

namespace OHOS {
namespace Media {
class HdiCodec : public IGstCodec, public NoCopyable, public std::enable_shared_from_this<HdiCodec> {
public:
    explicit HdiCodec(const std::string& component);
    virtual ~HdiCodec();
    int32_t Init() override;
    void SetHdiInBufferMgr(std::shared_ptr<HdiBufferMgr> bufferMgr);
    void SetHdiOutBufferMgr(std::shared_ptr<HdiBufferMgr> bufferMgr);
    void SetHdiParamsMgr(std::shared_ptr<HdiParamsMgr> paramsMgr);
    int32_t SetParameter(GstCodecParamKey key, GstElement *element) override;
    int32_t GetParameter(GstCodecParamKey key, GstElement *element) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t AllocateInputBuffers() override;
    int32_t UseInputBuffers(std::vector<GstBuffer*> buffers) override;
    int32_t PushInputBuffer(GstBuffer *buffer) override;
    int32_t PullInputBuffer(GstBuffer **buffer) override;
    int32_t FreeInputBuffers() override;
    int32_t AllocateOutputBuffers() override;
    int32_t UseOutputBuffers(std::vector<GstBuffer*> buffers) override;
    int32_t PushOutputBuffer(GstBuffer *buffer) override;
    int32_t PullOutputBuffer(GstBuffer **buffer) override;
    int32_t FreeOutputBuffers() override;
    int32_t Flush(GstCodecDirect direct) override;
    int32_t ActiveBufferMgr(GstCodecDirect direct, bool active) override;
    void Deinit() override;
private:
    struct AppData {
        std::weak_ptr<HdiCodec> instance;
    };
    enum PortState : int32_t {
        ACTIVATED,
        ACTIVING,
        DEACTIVATED,
        DEACTIVING,
    };
    AppData *appData_ = nullptr;
    CodecComponentType *handle_ = nullptr;
    CodecCallbackType *callback_ = nullptr;
    static int32_t Event(CodecCallbackType *self, OMX_EVENTTYPE event, EventInfo *info);
    static int32_t EmptyBufferDone(CodecCallbackType *self, int64_t appData, const OmxCodecBuffer *buffer);
    static int32_t FillBufferDone(CodecCallbackType *self, int64_t appData, const OmxCodecBuffer *buffer);
    void HandelEventFlush(OMX_U32 data);
    void HandelEventCmdComplete(OMX_U32 data1, OMX_U32 data2);
    void HandelEventStateSet(OMX_U32 data);
    void HandleEventPortSettingsChanged(OMX_U32 data1, OMX_U32 data2);
    void HandleEventBufferFlag(OMX_U32 data1, OMX_U32 data2);
    void HandleEventError(OMX_U32 data);
    void WaitForEvent(OMX_U32 cmd);
    int32_t WaitForState(OMX_STATETYPE state);
    void HandelEventPortDisable(OMX_U32 data);
    void HandelEventPortEnable(OMX_U32 data);
    int32_t ChangeState(OMX_STATETYPE state);
    void InitVersion();
    std::shared_ptr<HdiBufferMgr> inBufferMgr_;
    std::shared_ptr<HdiBufferMgr> outBufferMgr_;
    std::shared_ptr<HdiParamsMgr> paramsMgr_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::string componentName_ = "";
    OMX_PORT_PARAM_TYPE portParam_ = {};
    GstCodecRet ret_ = GST_CODEC_OK;
    bool eventDone_ = false;
    bool needCrop_ = true;
    int32_t lastCmd_ = -2; // -1 for error cmd and -2 for invaild
    OMX_STATETYPE curState_ = OMX_StateInvalid;
    OMX_STATETYPE targetState_ = OMX_StateInvalid;
    uint32_t inPortIndex_ = 0;
    uint32_t outPortIndex_ = 0;
    PortState inState_ = ACTIVATED;
    PortState outState_ = ACTIVATED;
    bool start_ = false;
    uint32_t id_ = 0;
    CompVerInfo verInfo_ = {};
};
} // namespace Media
} // namespace OHOS
#endif // OMX_CODEC_H
