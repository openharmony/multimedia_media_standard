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

#include "avmeta_frame_converter.h"
#include <gst/app/gstappsrc.h>
#include <gst/video/video-info.h>
#include "media_errors.h"
#include "media_log.h"
#include "gst_utils.h"
#include "gst_shmem_memory.h"
#include "scope_guard.h"
#include "time_perf.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetaFrameConv"};
    static const int32_t KEEP_ORIGINAL_WIDTH_OR_HEIGHT = -1;
}

namespace OHOS {
namespace Media {
struct PixelFormatInfo {
    PixelFormat format;
    std::string_view gstVideoFormat;
    uint8_t bytesPerPixel;
};

static const std::unordered_map<PixelFormat, PixelFormatInfo> PIXELFORMAT_INFO = {
    { PixelFormat::RGB_565, { PixelFormat::RGB_565, "RGB16", 2 } },
    { PixelFormat::RGB_888, { PixelFormat::RGB_888, "RGB", 3 } },
};

AVMetaFrameConverter::AVMetaFrameConverter()
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

AVMetaFrameConverter::~AVMetaFrameConverter()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    (void)Reset();
    CLEAN_PERF_RECORD(this);
}

int32_t AVMetaFrameConverter::Init(const OutputConfiguration &config)
{
    std::unique_lock<std::mutex> lock(mutex_);

    int32_t ret = SetupConvPipeline();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    ret = SetupConvSrc();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    ret = SetupConvSink(config);
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    ret = SetupMsgProcessor();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> AVMetaFrameConverter::Convert(GstCaps &inCaps, GstBuffer &inBuf)
{
    AUTO_PERF(this, "ConvertFrame");

    std::unique_lock<std::mutex> lock(mutex_);

    int32_t ret = PrepareConvert(inCaps);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "prepare convert failed");

    GstFlowReturn flowRet = GST_FLOW_ERROR;
    g_signal_emit_by_name(G_OBJECT(appSrc_), "push-buffer", &inBuf, &flowRet);
    CHECK_AND_RETURN_RET_LOG(flowRet == GST_FLOW_OK, nullptr, "push buffer failed");

    cond_.wait(lock, [this]() { return lastResult_ != nullptr || !startConverting_; });

    return GetConvertResult();
}

int32_t AVMetaFrameConverter::PrepareConvert(GstCaps &inCaps)
{
    ON_SCOPE_EXIT(0) { (void)GetConvertResult(); };

    if (lastCaps_ == nullptr || !gst_caps_is_equal(lastCaps_, &inCaps)) {
        MEDIA_LOGI("caps changed");
        MEDIA_LOGI("current caps: %{public}s", gst_caps_to_string(&inCaps));
        int32_t ret = ChangeState(GST_STATE_READY);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

        g_object_set(G_OBJECT(appSrc_), "caps", &inCaps,  nullptr);
        if (lastCaps_ != nullptr) {
            gst_caps_unref(lastCaps_);
        }
        lastCaps_ = gst_caps_ref(&inCaps);

        ret = ChangeState(GST_STATE_PLAYING);
        CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);
    }

    CANCEL_SCOPE_EXIT_GUARD(0);
    startConverting_ = true;

    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> AVMetaFrameConverter::GetConvertResult()
{
    startConverting_ = false;

    if (lastResult_ == nullptr) {
        MEDIA_LOGE("last result frame is nullptr");
        return nullptr;
    }

    ON_SCOPE_EXIT(0) {
        gst_buffer_unref(lastResult_);
        lastResult_ = nullptr;
    };

    GstMapInfo info = GST_MAP_INFO_INIT;
    gboolean ret = gst_buffer_map(lastResult_, &info, GST_MAP_READ);
    CHECK_AND_RETURN_RET_LOG(ret, nullptr, "map failed, exit convert frame");

    ON_SCOPE_EXIT(1) { gst_buffer_unmap(lastResult_, &info); };

    GstVideoMeta *videoMeta = gst_buffer_get_video_meta(lastResult_);
    CHECK_AND_RETURN_RET_LOG(videoMeta != nullptr, nullptr, "get video meta failed");

    GstShMemMemory *mem = reinterpret_cast<GstShMemMemory *>(info.memory);
    CHECK_AND_RETURN_RET_LOG(mem != nullptr && mem->mem != nullptr, nullptr, "mem is nullptr");
    CHECK_AND_RETURN_RET_LOG(mem->mem->GetBase() != nullptr, nullptr, "addr is nullptr");

    std::shared_ptr<AVSharedMemory> result = mem->mem;
    if (!(result->GetSize() > 0 && static_cast<uint32_t>(result->GetSize()) >= sizeof(OutputFrame))) {
        MEDIA_LOGE("size is incorrect");
        return nullptr;
    }

    auto frame = reinterpret_cast<OutputFrame *>(result->GetBase());
    frame->bytesPerPixel_ = PIXELFORMAT_INFO.at(outConfig_.colorFormat).bytesPerPixel;
    frame->width_ = static_cast<int32_t>(videoMeta->width);
    frame->height_ = static_cast<int32_t>(videoMeta->height);
    frame->stride_ = videoMeta->stride[0];
    frame->size_ = frame->stride_ * frame->height_;

    CHECK_AND_RETURN_RET_LOG(result->GetSize() >= frame->GetFlattenedSize(), nullptr, "size is incorrect");

    MEDIA_LOGI("======================Convert Frame Finished=========================");
    MEDIA_LOGI("output width = %{public}d, stride = %{public}d, height = %{public}d, format = %{public}d",
        frame->width_, frame->stride_, frame->height_, outConfig_.colorFormat);
    return result;
}

void AVMetaFrameConverter::UninstallPipeline()
{
    if (pipeline_ != nullptr) {
        ChangeState(GST_STATE_NULL);
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }

    if (pipeline_ != nullptr) {
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }

    if (vidShMemSink_ != nullptr) {
        gst_object_unref(vidShMemSink_);
        vidShMemSink_ = nullptr;
    }

    if (appSrc_ != nullptr) {
        gst_object_unref(appSrc_);
        appSrc_ = nullptr;
    }

    currState_ = GST_STATE_NULL;
}

int32_t AVMetaFrameConverter::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);

    startConverting_ = true;
    if (currState_ == GST_STATE_PAUSED) {
        cond_.wait(lock, [&]() { return currState_ == GST_STATE_PLAYING || !startConverting_; });
    }

    if (currState_ == GST_STATE_PLAYING) {
        MEDIA_LOGD("send eos");
        GstEvent *event = gst_event_new_eos();
        (void)gst_element_send_event(appSrc_, event);
        cond_.wait(lock, [&]() { return eosDone_ || !startConverting_; });
    }

    if (currState_ >= GST_STATE_PAUSED) {
        MEDIA_LOGD("change to state ready");
        ChangeState(GST_STATE_READY);
        cond_.wait(lock, [&]() { return currState_ == GST_STATE_READY || !startConverting_; });
    }

    /**
     * Must flush before change state and delete the msgProcessor, otherwise deadlock will
     * happend when try to destory the msgprocessor.
     */
    auto tempMsgProc = std::move(msgProcessor_);
    lock.unlock();
    tempMsgProc->FlushBegin();
    tempMsgProc->Reset();
    lock.lock();

    UninstallPipeline();

    if (lastResult_ != nullptr) {
        gst_buffer_unref(lastResult_);
        lastResult_ = nullptr;
    }

    if (lastCaps_ != nullptr) {
        gst_caps_unref(lastCaps_);
        lastCaps_ = nullptr;
    }

    for (auto &sample : allResults_) {
        gst_buffer_unref(sample);
    }
    allResults_.clear();

    startConverting_ = false;
    cond_.notify_all();
    eosDone_ = false;

    return MSERR_OK;
}

int32_t AVMetaFrameConverter::SetupConvPipeline()
{
    pipeline_ = GST_PIPELINE_CAST(gst_pipeline_new("conv_pipeline"));
    CHECK_AND_RETURN_RET(pipeline_ != nullptr, MSERR_INVALID_OPERATION);
    GstBin *bin = GST_BIN_CAST(pipeline_);

    appSrc_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("appsrc", "conv_src")));
    CHECK_AND_RETURN_RET(appSrc_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(gst_bin_add(bin, appSrc_), MSERR_INVALID_OPERATION);

    GstElement *scale = gst_element_factory_make("videoscale", "conv_scale");
    CHECK_AND_RETURN_RET(scale != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(gst_bin_add(bin, scale), MSERR_INVALID_OPERATION);

    GstElement *conv = gst_element_factory_make("videoconvert", "conv_conv");
    CHECK_AND_RETURN_RET(conv != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(gst_bin_add(bin, conv), MSERR_INVALID_OPERATION);

    vidShMemSink_ = GST_ELEMENT_CAST(gst_object_ref(gst_element_factory_make("vidshmemsink", "conv_sink")));
    CHECK_AND_RETURN_RET(vidShMemSink_ != nullptr, MSERR_INVALID_OPERATION);
    CHECK_AND_RETURN_RET(gst_bin_add(bin, vidShMemSink_), MSERR_INVALID_OPERATION);

    gboolean ret = gst_element_link_pads_full(appSrc_, "src", scale, "sink", GST_PAD_LINK_CHECK_NOTHING);
    CHECK_AND_RETURN_RET(ret, MSERR_INVALID_OPERATION);
    ret = gst_element_link_pads_full(scale, "src", conv, "sink", GST_PAD_LINK_CHECK_NOTHING);
    CHECK_AND_RETURN_RET(ret, MSERR_INVALID_OPERATION);
    ret = gst_element_link_pads_full(conv, "src", vidShMemSink_, "sink", GST_PAD_LINK_CHECK_NOTHING);
    CHECK_AND_RETURN_RET(ret, MSERR_INVALID_OPERATION);

    MEDIA_LOGI("setup converter pipeline success");
    return MSERR_OK;
}

int32_t AVMetaFrameConverter::SetupConvSrc()
{
    g_object_set(appSrc_, "is-live", TRUE, nullptr);
    g_object_set(appSrc_, "stream-type", GST_APP_STREAM_TYPE_STREAM, nullptr);
    g_object_set(appSrc_, "do-timestamp", TRUE, nullptr);
    return MSERR_OK;
}

int32_t AVMetaFrameConverter::SetupConvSink(const OutputConfiguration &outConfig)
{
    if (PIXELFORMAT_INFO.count(outConfig.colorFormat) == 0) {
        MEDIA_LOGE("pixelformat unsupported: %{public}d", outConfig.colorFormat);
        return MSERR_INVALID_VAL;
    }

    MEDIA_LOGI("target out config: width: %{public}d, height: %{public}d, format: %{public}d",
        outConfig.dstWidth, outConfig.dstHeight, outConfig.colorFormat);

    const char *formatStr = PIXELFORMAT_INFO.at(outConfig.colorFormat).gstVideoFormat.data();
    GstStructure *struc = gst_structure_new("video/x-raw", "format", G_TYPE_STRING, formatStr, nullptr);
    CHECK_AND_RETURN_RET(struc != nullptr, MSERR_NO_MEMORY);

    if (outConfig.dstHeight != KEEP_ORIGINAL_WIDTH_OR_HEIGHT) {
        gst_structure_set(struc, "height", G_TYPE_INT, outConfig.dstHeight, nullptr);
    }

    if (outConfig.dstWidth != KEEP_ORIGINAL_WIDTH_OR_HEIGHT) {
        gst_structure_set(struc, "width", G_TYPE_INT, outConfig.dstWidth, nullptr);
    }

    GstCaps *caps = gst_caps_new_full(struc, nullptr);
    CHECK_AND_RETURN_RET(caps != nullptr, MSERR_NO_MEMORY);

    g_object_set(G_OBJECT(vidShMemSink_), "caps", caps, nullptr);
    gst_caps_unref(caps);
    caps = nullptr;

    g_object_set(G_OBJECT(vidShMemSink_), "mem-prefix", sizeof(OutputFrame), nullptr);
    (void)g_signal_connect(G_OBJECT(vidShMemSink_), "new-sample", G_CALLBACK(OnNotifyNewSample), this);

    outConfig_ = outConfig;
    return MSERR_OK;
}

int32_t AVMetaFrameConverter::SetupMsgProcessor()
{
    GstBus *bus = gst_pipeline_get_bus(pipeline_);
    CHECK_AND_RETURN_RET_LOG(bus != nullptr, MSERR_UNKNOWN, "can not get bus");

    auto msgNotifier = std::bind(&AVMetaFrameConverter::OnNotifyMessage, this, std::placeholders::_1);
    msgProcessor_ = std::make_unique<GstMsgProcessor>(*bus, msgNotifier);
    gst_object_unref(bus);
    bus = nullptr;

    int32_t ret = msgProcessor_->Init();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    // only concern the msg from pipeline
    msgProcessor_->AddMsgFilter(ELEM_NAME(GST_ELEMENT_CAST(pipeline_)));

    return MSERR_OK;
}

int32_t AVMetaFrameConverter::ChangeState(GstState targetState)
{
    MEDIA_LOGI("begin change state to %{public}s", gst_element_state_get_name(targetState));

    GstStateChangeReturn stateRet = gst_element_set_state(GST_ELEMENT_CAST(pipeline_), targetState);
    if (stateRet == GST_STATE_CHANGE_FAILURE) {
        MEDIA_LOGE("change conv pipeline to ready failed");
        return MSERR_INVALID_OPERATION;
    }

    return MSERR_OK;
}

void AVMetaFrameConverter::OnNotifyMessage(const InnerMessage &msg)
{
    MEDIA_LOGD("msg: %{public}d", msg.type);
    std::unique_lock<std::mutex> lock(mutex_);

    switch (msg.type) {
        case InnerMsgType::INNER_MSG_STATE_CHANGED: {
            MEDIA_LOGD("state is %{public}s", gst_element_state_get_name(currState_));
            currState_ = static_cast<GstState>(msg.detail2);
            cond_.notify_all();
            break;
        }
        case InnerMsgType::INNER_MSG_ERROR: {
            startConverting_ = false;
            MEDIA_LOGE("error happened");
            cond_.notify_all();
            break;
        }
        case InnerMsgType::INNER_MSG_EOS: {
            eosDone_ = true;
            MEDIA_LOGD("eos done");
            cond_.notify_all();
            break;
        }
        default:
            break;
    }
}

GstFlowReturn AVMetaFrameConverter::OnNotifyNewSample(GstElement *elem, AVMetaFrameConverter *thiz)
{
    CHECK_AND_RETURN_RET(thiz != nullptr, GST_FLOW_ERROR);
    CHECK_AND_RETURN_RET(elem != nullptr, GST_FLOW_ERROR);

    std::unique_lock<std::mutex> lock(thiz->mutex_);
    if (thiz->lastResult_ != nullptr) {
        gst_buffer_unref(thiz->lastResult_);
        thiz->lastResult_ = nullptr;
    }

    GstSample *sample = nullptr;
    g_signal_emit_by_name(elem, "pull-sample", &sample);
    CHECK_AND_RETURN_RET(sample != nullptr, GST_FLOW_ERROR);

    ON_SCOPE_EXIT(0) { gst_sample_unref(sample); };

    GstBuffer *buffer =  gst_sample_get_buffer(sample);
    CHECK_AND_RETURN_RET(buffer != nullptr, GST_FLOW_ERROR);
    MEDIA_LOGI("sample buffer arrived, pts: %{public}" PRIu64 "", GST_BUFFER_PTS(buffer));

    thiz->lastResult_ = gst_buffer_ref(buffer);
    CHECK_AND_RETURN_RET(thiz->lastResult_ != nullptr, GST_FLOW_ERROR);

    // increase the refcount to avoid the buffer to be released to bufferpool.
    thiz->allResults_.push_back(gst_buffer_ref(buffer));

    thiz->cond_.notify_all();
    return GST_FLOW_OK;
}
}
}
