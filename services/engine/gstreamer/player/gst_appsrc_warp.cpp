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

#include "gst_appsrc_warp.h"
#include "media_log.h"
#include "media_errors.h"
#include "player.h"
#include "securec.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstAppsrcWarp"};
}

namespace OHOS {
namespace Media {
std::shared_ptr<GstAppsrcWarp> GstAppsrcWarp::Create(const std::shared_ptr<IMediaDataSource> &dataSrc)
{
    CHECK_AND_RETURN_RET_LOG(dataSrc != nullptr, nullptr, "input dataSrc is empty!");
    int64_t size = 0;
    int32_t ret = dataSrc->GetSize(size);
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "media data source get size failed!");
    CHECK_AND_RETURN_RET_LOG(size >= -1, nullptr, "size cannot less than -1");
    return std::make_shared<GstAppsrcWarp>(dataSrc, size);
}

GstAppsrcWarp::GstAppsrcWarp(const std::shared_ptr<IMediaDataSource> &dataSrc, const int64_t size)
    : dataSrc_(dataSrc),
      size_(size),
      taskQue_("appsrcWarpTask")
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create and size %{public}" PRId64 "", FAKE_POINTER(this), size);
    (void)taskQue_.Start();
    streamType_ = size == -1 ? GST_APP_STREAM_TYPE_STREAM : GST_APP_STREAM_TYPE_RANDOM_ACCESS;
}

GstAppsrcWarp::~GstAppsrcWarp()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    (void)taskQue_.Stop();
    ClearAppsrc();
}

void GstAppsrcWarp::ClearAppsrc()
{
    if (appSrc_ != nullptr) {
        for (auto &id : callbackIds_) {
            g_signal_handler_disconnect(appSrc_, id);
        }
        callbackIds_.clear();
        gst_object_unref(appSrc_);
        appSrc_ = nullptr;
    }
}

int32_t GstAppsrcWarp::SetAppsrc(GstElement *appSrc)
{
    MEDIA_LOGD("set Appsrc");
    ClearAppsrc();
    appSrc_ = static_cast<GstElement *>(gst_object_ref(appSrc));
    CHECK_AND_RETURN_RET_LOG(appSrc_ != nullptr, MSERR_INVALID_VAL, "gstPlayer_ is nullptr");
    SetCallBackForAppSrc();
    return MSERR_OK;
}

void GstAppsrcWarp::SetCallBackForAppSrc()
{
    MEDIA_LOGD("SetCallBackForAppSrc");
    CHECK_AND_RETURN_LOG(appSrc_ != nullptr, "appSrc_ is nullptr");
    int64_t size = static_cast<int64_t>(size_);
    g_object_set(appSrc_, "stream-type", streamType_, nullptr);
    g_object_set(appSrc_, "format", GST_FORMAT_BYTES, nullptr);
    g_object_set(appSrc_, "size", size, nullptr);
    callbackIds_.push_back(g_signal_connect(appSrc_, "need-data", G_CALLBACK(NeedData), this));
    if (streamType_ == GST_APP_STREAM_TYPE_RANDOM_ACCESS) {
        callbackIds_.push_back(g_signal_connect(appSrc_, "seek-data", G_CALLBACK(SeekData), this));
    }
    MEDIA_LOGD("setcall back end");
}

bool GstAppsrcWarp::NoSeek() const
{
    return streamType_ == GST_APP_STREAM_TYPE_STREAM;
}

int32_t GstAppsrcWarp::SetErrorCallback(const std::weak_ptr<IPlayerEngineObs> &obs)
{
    CHECK_AND_RETURN_RET_LOG(obs.lock() != nullptr, MSERR_INVALID_OPERATION,
        "obs is nullptr, please set playercallback");
    obs_ = obs;
    return MSERR_OK;
}

void GstAppsrcWarp::NeedData(const GstElement *appSrc, uint32_t size, gpointer self)
{
    MEDIA_LOGD("needData size: %{public}u", size);
    (void)appSrc;
    CHECK_AND_RETURN_LOG(self != nullptr, "self is nullptr");
    auto warp = static_cast<GstAppsrcWarp *>(self);
    warp->NeedDataInner(size);
}

void GstAppsrcWarp::NeedDataInner(uint32_t size)
{
    std::unique_lock<std::mutex> lock(mutex_);
    targetPos_ = curPos_ + size;
    auto task = std::make_shared<TaskHandler<void>>([this] {
        ReadAndGetMem();
    });
    CHECK_AND_RETURN_LOG(taskQue_.EnqueueTask(task) == MSERR_OK, "enque task failed");
}

gboolean GstAppsrcWarp::SeekData(const GstElement *appSrc, uint64_t seekPos, gpointer self)
{
    MEDIA_LOGD("SeekData pos: %{public}" PRIu64 "", seekPos);
    (void)appSrc;
    CHECK_AND_RETURN_RET_LOG(self != nullptr, FALSE, "self is nullptr");
    auto warp = static_cast<GstAppsrcWarp *>(self);
    return warp->SeekDataInner(seekPos);
}

gboolean GstAppsrcWarp::SeekDataInner(uint64_t pos)
{
    std::unique_lock<std::mutex> lock(mutex_);
    curPos_ = pos;
    targetPos_ = pos;
    return TRUE;
}

void GstAppsrcWarp::ReadAndGetMem()
{
    std::unique_lock<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(targetPos_ >= curPos_, "targetpos should larger than curpos error");
    if (targetPos_ == curPos_) {
        return;
    }
    int32_t size = 0;
    uint32_t len = static_cast<uint32_t>(targetPos_ - curPos_);
    if (size_ == -1) {
        size = dataSrc_->ReadAt(len);
    } else {
        size = dataSrc_->ReadAt(static_cast<int64_t>(curPos_), len);
    }

    AnalyzeSize(size);
}

void GstAppsrcWarp::AnalyzeSize(int32_t size)
{
    if (size == 0) {
        return;
    }
    if (size > 0) {
        int32_t ret = GetAndPushMem(size);
        if (ret != MSERR_OK) {
            OnError(ret);
        }
        return;
    }
    targetPos_ = curPos_;
    PushEos();
    switch (size) {
        case SOURCE_ERROR_IO:
            OnError(MSERR_DATA_SOURCE_IO_ERROR);
            MEDIA_LOGW("IO ERROR %d", size);
            break;
        case SOURCE_ERROR_EOF:
            break;
        default:
            OnError(MSERR_DATA_SOURCE_ERROR_UNKNOWN);
            MEDIA_LOGE("unknow error %d", size);
            break;
    }
}

int32_t GstAppsrcWarp::GetAndPushMem(int32_t size)
{
    CHECK_AND_RETURN_RET_LOG(size > 0, MSERR_NO_MEMORY, "size should biger than zero");
    GstBuffer *buffer = nullptr;
    std::shared_ptr<AVSharedMemory> mem = dataSrc_->GetMem();
    CHECK_AND_RETURN_RET_LOG(mem != nullptr, MSERR_DATA_SOURCE_OBTAIN_MEM_ERROR, "get mem is nullptr");
    CHECK_AND_RETURN_RET_LOG(mem->GetBase() != nullptr, MSERR_DATA_SOURCE_OBTAIN_MEM_ERROR, "mem base is nullptr");
    if (size > mem->GetSize()) {
        size = mem->GetSize();
    }
    buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, MSERR_NO_MEMORY, "no mem error");
    GstMapInfo info = GST_MAP_INFO_INIT;
    if (gst_buffer_map(buffer, &info, GST_MAP_WRITE) == FALSE) {
        gst_buffer_unref(buffer);
        MEDIA_LOGE("map buffer failed");
        return MSERR_NO_MEMORY;
    }
    if (memcpy_s(info.data, static_cast<size_t>(size), mem->GetBase(), static_cast<size_t>(mem->GetSize())) != EOK) {
        gst_buffer_unmap(buffer, &info);
        gst_buffer_unref(buffer);
        MEDIA_LOGE("memcpy_s failed");
        return MSERR_DATA_SOURCE_OBTAIN_MEM_ERROR;
    }
    gst_buffer_unmap(buffer, &info);
    GST_BUFFER_OFFSET(buffer) = curPos_;
    curPos_ += size;
    PushData(buffer);
    gst_buffer_unref(buffer);
    buffer = nullptr;

    return MSERR_OK;
}

void GstAppsrcWarp::OnError(int32_t errorCode)
{
    PlayerErrorType errorType = PLAYER_ERROR_UNKNOWN;
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        tempObs->OnError(errorType, errorCode);
    }
}

void GstAppsrcWarp::PushData(void *buffer)
{
    int32_t ret = GST_FLOW_OK;
    if (appSrc_ != nullptr) {
        g_signal_emit_by_name(appSrc_, "push-buffer", buffer, &ret);
    }

    MEDIA_LOGD("appsrcPushData ret:%{public}d", ret);
}

void GstAppsrcWarp::PushEos()
{
    int32_t ret = GST_FLOW_OK;
    if (appSrc_ != nullptr) {
        g_signal_emit_by_name(appSrc_, "end-of-stream", &ret);
    }
    MEDIA_LOGD("appsrcPushData ret:%{public}d", ret);
}
} // namespace Media
} // namespace OHOS