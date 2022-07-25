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
static const std::unordered_map<int32_t, IPlayBinCtrler::PlayBinSeekMode> SEEK_OPTION_MAPPING = {
    { AV_META_QUERY_NEXT_SYNC, IPlayBinCtrler::PlayBinSeekMode::NEXT_SYNC },
    { AV_META_QUERY_PREVIOUS_SYNC, IPlayBinCtrler::PlayBinSeekMode::PREV_SYNC },
    { AV_META_QUERY_CLOSEST_SYNC, IPlayBinCtrler::PlayBinSeekMode::CLOSET_SYNC },
    { AV_META_QUERY_CLOSEST, IPlayBinCtrler::PlayBinSeekMode::CLOSET },
};

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
    StopExtract();

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
    seekDone_ = false;
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
    static constexpr int32_t timeout = 5;
    cond_.wait_for(lock, std::chrono::seconds(timeout), [this]() {
        return !originalFrames_.empty() || !startExtracting_;
    });
    CHECK_AND_RETURN_RET_LOG(startExtracting_, outFrames, "cancelled, exit frame extract");
    CHECK_AND_RETURN_RET_LOG(!originalFrames_.empty(), outFrames, "no more frames");

    auto item = originalFrames_.front();
    originalFrames_.pop();
    auto frameConverter = std::move(frameConverter_);
    lock.unlock();

    auto outFrame = frameConverter->Convert(*item.second, *item.first);
    if (outFrame == nullptr) {
        gst_buffer_unref(item.first);
        gst_caps_unref(item.second);
        MEDIA_LOGE("convert frame failed");
        return outFrames;
    }
    gst_buffer_unref(item.first);
    gst_caps_unref(item.second);

    outFrames.push_back(outFrame);
    MEDIA_LOGD("extract frame success, frame number: %{public}zu", outFrames.size());
    return outFrames;
}

int32_t AVMetaFrameExtractor::StartExtract(
    int32_t numFrames, int64_t timeUs, int32_t option, const OutputConfiguration &param)
{
    (void)numFrames;
    std::unique_lock<std::mutex> lock(mutex_);

    ON_SCOPE_EXIT(0) { StopExtract(); };

    ClearCache();
    startExtracting_ = true;

    IPlayBinCtrler::PlayBinSeekMode mode = IPlayBinCtrler::PlayBinSeekMode::PREV_SYNC;
    if (SEEK_OPTION_MAPPING.find(option) != SEEK_OPTION_MAPPING.end()) {
        mode = SEEK_OPTION_MAPPING.at(option);
    }

    int32_t ret = playbin_->Seek(timeUs, mode);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "seek failed, cancel extract frames");

    frameConverter_ = std::make_unique<AVMetaFrameConverter>();
    ret = frameConverter_->Init(param);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "init failed, cancel extract frames");

    static constexpr int32_t timeout = 5;
    cond_.wait_for(lock, std::chrono::seconds(timeout), [this]() {
        return seekDone_ || !startExtracting_;
    });
    CHECK_AND_RETURN_RET(startExtracting_, MSERR_INVALID_OPERATION);

    // no next sync frame, change to find the prev sync frame
    if (originalFrames_.empty() && option == AVMetadataQueryOption::AV_META_QUERY_NEXT_SYNC) {
        ClearCache();
        mode = IPlayBinCtrler::PlayBinSeekMode::PREV_SYNC;
        ret = playbin_->Seek(timeUs, mode);
        CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, ret, "sek failed, cancel extract frames");
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

void AVMetaFrameExtractor::NotifyPlayBinMsg(const PlayBinMessage &msg)
{
    std::unique_lock<std::mutex> lock(mutex_);

    switch (msg.type) {
        case PlayBinMsgType::PLAYBIN_MSG_SEEKDONE: {
            seekDone_ = true;
            cond_.notify_all();
            break;
        }
        default:
            break;
    }
}

int32_t AVMetaFrameExtractor::SetupVideoSink()
{
    g_object_set(G_OBJECT(vidAppSink_), "emit-signals", TRUE, nullptr);
    g_object_set(G_OBJECT(vidAppSink_), "max-buffers", 1, nullptr);

    gulong signalId = g_signal_connect(G_OBJECT(vidAppSink_), "new-preroll", G_CALLBACK(OnNewPrerollArrived), this);
    CHECK_AND_RETURN_RET_LOG(signalId != 0, MSERR_INVALID_OPERATION, "listen to new-preroll failed");
    signalIds_.push_back(signalId);

    return MSERR_OK;
}

GstFlowReturn AVMetaFrameExtractor::OnNewPrerollArrived(GstElement *sink, AVMetaFrameExtractor *thiz)
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

    thiz->cond_.notify_all();
    return GST_FLOW_OK;
}
} // namespace Media
} // namespace OHOS
