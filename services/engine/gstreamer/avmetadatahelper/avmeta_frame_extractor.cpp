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

#include "avmeta_frame_extractor.h"
#include "media_errors.h"
#include "media_log.h"
#include "scope_guard.h"
#include "time_perf.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetaFrameExtract"};
}

namespace OHOS {
namespace Media {
AVMetaFrameExtractor::AVMetaFrameExtractor()
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

AVMetaFrameExtractor::~AVMetaFrameExtractor()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    Reset();
}

int32_t AVMetaFrameExtractor::Init(const std::shared_ptr<IPlayBinCtrler> &playbin, GstElement &vidAppSink)
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (playbin == nullptr) {
        MEDIA_LOGE("playbin is nullptr");
        return MSERR_INVALID_VAL;
    }
    playbin_ = playbin;
    vidAppSink_ = GST_ELEMENT_CAST(gst_object_ref(&vidAppSink));

    int32_t ret = SetupVideoSink();
    CHECK_AND_RETURN_RET(ret == MSERR_OK, ret);

    return MSERR_OK;
}

std::shared_ptr<AVSharedMemory> AVMetaFrameExtractor::ExtractFrame(
    int64_t timeUs, int32_t option, const OutputConfiguration &param)
{
    int32_t ret = StartExtract(1, timeUs, option, param);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "start extract failed");

    auto outFrames = ExtractInternel();
    CHECK_AND_RETURN_RET_LOG(!outFrames.empty(), nullptr, "extract failed");

    return outFrames[0];
}

void AVMetaFrameExtractor::ClearCache()
{
    while (!originalFrames_.empty()) {
        auto item = originalFrames_.front();
        originalFrames_.pop();
        gst_buffer_unref(item.first);
        gst_caps_unref(item.second);
    }
    currOriginalFrameCount_ = 0;
}

void AVMetaFrameExtractor::Reset()
{
    std::unique_lock<std::mutex> lock(mutex_);

    StopExtract();

    decltype(signalIds_) tempSignalIds;
    tempSignalIds.swap(signalIds_);

    lock.unlock();
    for(auto signalId : tempSignalIds) {
        g_signal_handler_disconnect(vidAppSink_, signalId);
    }
    lock.lock();

    if (vidAppSink_ != nullptr) {
        gst_object_unref(vidAppSink_);
        vidAppSink_ = nullptr;
    }
    playbin_ = nullptr;
}

std::vector<std::shared_ptr<AVSharedMemory>> AVMetaFrameExtractor::ExtractInternel()
{
    std::vector<std::shared_ptr<AVSharedMemory>> outFrames;

    std::unique_lock<std::mutex> lock(mutex_);
    auto frameConverter = std::move(frameConverter_);

    do {
        cond_.wait(lock, [this]() { return !originalFrames_.empty() || !startExtracting_; });
        CHECK_AND_BREAK_LOG(startExtracting_, "cancelled, exit frame extract");

        auto item = originalFrames_.front();
        lock.unlock();

        auto outFrame = frameConverter->Convert(*item.second, *item.first);
        CHECK_AND_BREAK_LOG(outFrame != nullptr, "convert frame failed");

        outFrames.push_back(outFrame);
        MEDIA_LOGD("extract frame success, frame number: %{public}zu", outFrames.size());

        gst_buffer_unref(item.first);
        gst_caps_unref(item.second);

        lock.lock();
        originalFrames_.pop();
    } while (outFrames.size() < static_cast<size_t>(maxFrames_));

    MEDIA_LOGD("extract frame finished");
    StopExtract();
    return outFrames;
}

int32_t AVMetaFrameExtractor::StartExtract(
    int32_t numFrames, int64_t timeUs, int32_t option, const OutputConfiguration &param)
{
    std::unique_lock<std::mutex> lock(mutex_);

    ON_SCOPE_EXIT(0) { StopExtract(); };

    ClearCache();
    startExtracting_ = true;
    maxFrames_ = numFrames;

    int32_t ret = playbin_->Seek(timeUs, option);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "seek failed, cancel extract frames");

    frameConverter_ = std::make_unique<AVMetaFrameConverter>();
    ret = frameConverter_->Init(param);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "init failed, cancel extract frames");

    if (numFrames > 1) {
        ret = playbin_->Play(); // play to generate more frames
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "play the pipeline failed");
    }

    CANCEL_SCOPE_EXIT_GUARD(0);
    return MSERR_OK;
}

void AVMetaFrameExtractor::StopExtract()
{
    ClearCache();
    startExtracting_ = false;
    cond_.notify_all();
    frameConverter_ = nullptr;
}

int32_t AVMetaFrameExtractor::SetupVideoSink()
{
    g_object_set(G_OBJECT(vidAppSink_), "emit-signals", TRUE, nullptr);
    g_object_set(G_OBJECT(vidAppSink_), "max-buffers", 1, nullptr);

    gulong signalId = g_signal_connect(G_OBJECT(vidAppSink_), "new-preroll", G_CALLBACK(OnNewPrerollArrived), this);
    CHECK_AND_RETURN_RET_LOG(signalId != 0, MSERR_INVALID_OPERATION, "listen to new-preroll failed");
    signalIds_.push_back(signalId);

    signalId = g_signal_connect(G_OBJECT(vidAppSink_), "new-sample", G_CALLBACK(OnNewSampleArrived), this);
    CHECK_AND_RETURN_RET_LOG(signalId != 0, MSERR_INVALID_OPERATION, "listen to new-sample failed");
    signalIds_.push_back(signalId);

    return MSERR_OK;
}

GstFlowReturn AVMetaFrameExtractor::OnNewPrerollArrived(
    GstElement *sink, AVMetaFrameExtractor *thiz)
{
    CHECK_AND_RETURN_RET(thiz != nullptr, GST_FLOW_ERROR);
    CHECK_AND_RETURN_RET(sink != nullptr, GST_FLOW_ERROR);

    std::unique_lock<std::mutex> lock(thiz->mutex_);

    GstSample *sample = nullptr;
    g_signal_emit_by_name(sink, "pull-preroll", &sample);
    CHECK_AND_RETURN_RET(sample != nullptr, GST_FLOW_ERROR);

    ON_SCOPE_EXIT(0) { gst_sample_unref(sample); };

    GstBuffer *buffer =  gst_sample_get_buffer(sample);
    CHECK_AND_RETURN_RET(buffer != nullptr, GST_FLOW_ERROR);
    MEDIA_LOGI("preroll buffer arrived, pts: %{public}" PRIu64 "", GST_BUFFER_PTS(buffer));

    if (!thiz->startExtracting_) {
        MEDIA_LOGI("not start extract, ignore");
        return GST_FLOW_OK;
    }

    GstCaps *caps = gst_sample_get_caps(sample);
    CHECK_AND_RETURN_RET(caps != nullptr, GST_FLOW_ERROR);
    thiz->originalFrames_.push({ gst_buffer_ref(buffer), gst_caps_ref(caps) });
    thiz->currOriginalFrameCount_ += 1;

    thiz->cond_.notify_all();
    return GST_FLOW_OK;
}

GstFlowReturn AVMetaFrameExtractor::OnNewSampleArrived(
    GstElement *sink, AVMetaFrameExtractor *thiz)
{
    CHECK_AND_RETURN_RET(thiz != nullptr, GST_FLOW_ERROR);
    CHECK_AND_RETURN_RET(sink != nullptr, GST_FLOW_ERROR);

    std::unique_lock<std::mutex> lock(thiz->mutex_);

    GstSample *sample = nullptr;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    CHECK_AND_RETURN_RET(sample != nullptr, GST_FLOW_ERROR);

    ON_SCOPE_EXIT(0) { gst_sample_unref(sample); };

    GstBuffer *buffer =  gst_sample_get_buffer(sample);
    CHECK_AND_RETURN_RET(buffer != nullptr, GST_FLOW_ERROR);
    MEDIA_LOGI("sample buffer arrived, pts: %{public}" PRIu64 "", GST_BUFFER_PTS(buffer));

    if (thiz->currOriginalFrameCount_ == 1) {
        MEDIA_LOGE("first sample, ignored"); // first sample is same with the preroll buffer
        return GST_FLOW_OK;
    }

    GstCaps *caps = gst_sample_get_caps(sample);
    CHECK_AND_RETURN_RET(caps != nullptr, GST_FLOW_ERROR);
    thiz->originalFrames_.push({ gst_buffer_ref(buffer), gst_caps_ref(caps) });
    thiz->currOriginalFrameCount_ += 1;

    if (thiz->currOriginalFrameCount_ >= thiz->maxFrames_) {
        (void)thiz->playbin_->Pause();
    }

    thiz->cond_.notify_all();
    return GST_FLOW_OK;
}
}
}
