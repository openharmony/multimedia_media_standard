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
    constexpr int32_t BUFFERS_NUM = 5;
    constexpr int32_t BUFFER_SIZE = 81920;
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
    std::shared_ptr<GstAppsrcWarp> warp = std::make_shared<GstAppsrcWarp>(dataSrc, size);
    CHECK_AND_RETURN_RET_LOG(warp->Init() == MSERR_OK, nullptr, "init failed");
    return warp;
}

GstAppsrcWarp::GstAppsrcWarp(const std::shared_ptr<IMediaDataSource> &dataSrc, const int64_t size)
    : dataSrc_(dataSrc),
      size_(size),
      fillTaskQue_("fillbufferTask"),
      emptyTaskQue_("emptybufferTask"),
      bufferSize_(BUFFER_SIZE),
      buffersNum_(BUFFERS_NUM)
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create and size %{public}" PRId64 "", FAKE_POINTER(this), size);
    streamType_ = size == -1 ? GST_APP_STREAM_TYPE_STREAM : GST_APP_STREAM_TYPE_RANDOM_ACCESS;
}

GstAppsrcWarp::~GstAppsrcWarp()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    {
        std::unique_lock<std::mutex> lock(mutex_);
        isExit_ = true;
        fillCond_.notify_all();
        emptyCond_.notify_all();
    }
    (void)fillTaskQue_.Stop();
    (void)emptyTaskQue_.Stop();
    ClearAppsrc();
}

int32_t GstAppsrcWarp::Init()
{
    for (int i = 0; i < buffersNum_; ++i) {
        std::shared_ptr<AppsrcMemWarp> appSrcMem = std::make_shared<AppsrcMemWarp>();
        CHECK_AND_RETURN_RET_LOG(appSrcMem != nullptr, MSERR_NO_MEMORY, "init AppsrcMemWarp failed");
        appSrcMem->mem = AVSharedMemory::Create(bufferSize_, AVSharedMemory::Flags::FLAGS_READ_WRITE, "appsrc");
        CHECK_AND_RETURN_RET_LOG(appSrcMem->mem != nullptr, MSERR_NO_MEMORY, "init AVSharedMemory failed");
        emptyBuffers_.emplace(appSrcMem);
    }
    CHECK_AND_RETURN_RET_LOG(fillTaskQue_.Start() == MSERR_OK, MSERR_INVALID_OPERATION, "init task failed");
    CHECK_AND_RETURN_RET_LOG(emptyTaskQue_.Start() == MSERR_OK, MSERR_INVALID_OPERATION, "init task failed");
    auto task = std::make_shared<TaskHandler<void>>([this] {
        FillTask();
    });
    CHECK_AND_RETURN_RET_LOG(fillTaskQue_.EnqueueTask(task) == MSERR_OK,
        MSERR_INVALID_OPERATION, "enque task failed");
    task = std::make_shared<TaskHandler<void>>([this] {
        EmptyTask();
    });
    CHECK_AND_RETURN_RET_LOG(emptyTaskQue_.EnqueueTask(task) == MSERR_OK,
        MSERR_INVALID_OPERATION, "enque task failed");
    return MSERR_OK;
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

bool GstAppsrcWarp::IsLiveMode() const
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
    (void)appSrc;
    CHECK_AND_RETURN_LOG(self != nullptr, "self is nullptr");
    auto warp = static_cast<GstAppsrcWarp *>(self);
    warp->NeedDataInner(size);
}

void GstAppsrcWarp::NeedDataInner(uint32_t size)
{
    std::unique_lock<std::mutex> lock(mutex_);
    int32_t ret = MSERR_OK;
    needDataSize_ = size;
    while (needDataSize_ > bufferSize_ * (buffersNum_ - 1)) {
        ret = MSERR_NO_MEMORY;
        ++buffersNum_;
        std::shared_ptr<AppsrcMemWarp> appSrcMem = std::make_shared<AppsrcMemWarp>();
        CHECK_AND_BREAK_LOG(appSrcMem != nullptr, "init AppsrcMemWarp failed");
        appSrcMem->mem = AVSharedMemory::Create(bufferSize_, AVSharedMemory::Flags::FLAGS_READ_WRITE, "appsrc");
        CHECK_AND_BREAK_LOG(appSrcMem->mem != nullptr, "init AVSharedMemory failed");
        emptyBuffers_.emplace(appSrcMem);
        ret = MSERR_OK;
    }
    if (ret != MSERR_OK) {
        OnError(ret);
    }
    if (!filledBuffers_.empty() && (needDataSize_ <= filledBufferSize_ || atEos_)) {
        ret = GetAndPushMem();
        if (ret != MSERR_OK) {
            OnError(ret);
        }
    } else {
        needData_ = true;
    }
}

void GstAppsrcWarp::FillTask()
{
    int32_t ret = ReadAndGetMem();
    if (ret != MSERR_OK) {
        OnError(ret);
    }
}

void GstAppsrcWarp::EmptyTask()
{
    int32_t ret = MSERR_OK;
    while (ret == MSERR_OK) {
        std::unique_lock<std::mutex> lock(mutex_);
        emptyCond_.wait(lock, [this] {
            return (!filledBuffers_.empty() && needData_ && (needDataSize_ <= filledBufferSize_ || atEos_)) || isExit_;
        });
        if (isExit_) {
            break;
        }
        ret = GetAndPushMem();
    }
    if (ret != MSERR_OK) {
        OnError(ret);
    }
}

gboolean GstAppsrcWarp::SeekData(const GstElement *appSrc, uint64_t seekPos, gpointer self)
{
    MEDIA_LOGD("SeekData pos: %{public}" PRIu64 "", seekPos);
    (void)appSrc;
    CHECK_AND_RETURN_RET_LOG(self != nullptr, FALSE, "self is nullptr");
    auto warp = static_cast<GstAppsrcWarp *>(self);
    return warp->SeekDataInner(seekPos);
}

void GstAppsrcWarp::SeekAndFreeBuffers(uint64_t pos)
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (!filledBuffers_.empty()) {
        std::shared_ptr<AppsrcMemWarp> appSrcMem = filledBuffers_.front();
        if (appSrcMem->size < 0) {
            filledBuffers_.pop();
            emptyBuffers_.push(appSrcMem);
            continue;
        }
        if (appSrcMem->pos <= pos && appSrcMem->pos + appSrcMem->size > pos) {
            uint32_t len = pos - appSrcMem->pos;
            filledBufferSize_ += appSrcMem->offset;
            appSrcMem->offset = len;
            filledBufferSize_ -= appSrcMem->offset;
            break;
        }
        filledBufferSize_ = filledBufferSize_ - (appSrcMem->size - appSrcMem->offset);
        filledBuffers_.pop();
        emptyBuffers_.push(appSrcMem);
    }
    if (filledBuffers_.empty()) {
        curPos_ = pos;
        atEos_ = false;
    }
    fillCond_.notify_all();
}

gboolean GstAppsrcWarp::SeekDataInner(uint64_t pos)
{
    SeekAndFreeBuffers(pos);
    return TRUE;
}

int32_t GstAppsrcWarp::ReadAndGetMem()
{
    int32_t ret = MSERR_OK;
    while (ret == MSERR_OK) {
        int32_t size = 0;
        std::shared_ptr<AppsrcMemWarp> appSrcMem = nullptr;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            fillCond_.wait(lock, [this] { return (!emptyBuffers_.empty() && !atEos_) || isExit_; });
            if (isExit_) {
                break;
            }
            appSrcMem = emptyBuffers_.front();
            CHECK_AND_RETURN_RET_LOG(appSrcMem != nullptr && appSrcMem->mem != nullptr, MSERR_NO_MEMORY, "no mem");
            appSrcMem->pos = curPos_;
            emptyBuffers_.pop();
        }
        if (size_ == -1) {
            size = dataSrc_->ReadAt(bufferSize_, appSrcMem->mem);
        } else {
            size = dataSrc_->ReadAt(static_cast<int64_t>(appSrcMem->pos), bufferSize_, appSrcMem->mem);
        }
        if (size > appSrcMem->mem->GetSize()) {
            ret = MSERR_INVALID_VAL;
        }
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (size == 0 || curPos_ != appSrcMem->pos) {
                emptyBuffers_.push(appSrcMem);
            } else if (size < 0) {
                appSrcMem->size = size;
                atEos_ = true;
                filledBuffers_.push(appSrcMem);
            } else {
                size = std::min(size, appSrcMem->mem->GetSize());
                appSrcMem->size = size;
                filledBufferSize_ += size;
                appSrcMem->pos = curPos_;
                appSrcMem->offset = 0;
                curPos_ = curPos_ + size;
                filledBuffers_.push(appSrcMem);
            }
            emptyCond_.notify_all();
        }
    }
    return ret;
}

void GstAppsrcWarp::EosAndCheckSize(int32_t size)
{
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

int32_t GstAppsrcWarp::GetAndPushMem()
{
    int32_t ret = MSERR_OK;
    int32_t size = needDataSize_;
    std::shared_ptr<AppsrcMemWarp> appSrcMem = filledBuffers_.front();
    CHECK_AND_RETURN_RET_LOG(appSrcMem != nullptr && appSrcMem->mem != nullptr, MSERR_NO_MEMORY, "no mem");
    size = size > filledBufferSize_ ? filledBufferSize_ : size;
    if (size == 0) {
        EosAndCheckSize(appSrcMem->size);
        filledBuffers_.pop();
        emptyBuffers_.push(appSrcMem);
        needData_ = false;
        return ret;
    }
    GstBuffer *buffer = nullptr;
    buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
    GST_BUFFER_OFFSET(buffer) = appSrcMem->pos + appSrcMem->offset;
    GstMapInfo info = GST_MAP_INFO_INIT;
    if (gst_buffer_map(buffer, &info, GST_MAP_WRITE) == FALSE) {
        gst_buffer_unref(buffer);
        MEDIA_LOGE("map buffer failed");
        ret = MSERR_NO_MEMORY;
        return ret;
    }
    bool copyRet = CopyToGstBuffer(info);
    gst_buffer_unmap(buffer, &info);
    if (!copyRet) {
        MEDIA_LOGE("copy buffer failed");
        gst_buffer_unref(buffer);
        ret = MSERR_NO_MEMORY;
        return ret;
    }
    PushData(buffer);
    filledBufferSize_ -= needDataSize_;
    needDataSize_ = 0;
    needData_ = false;
    gst_buffer_unref(buffer);
    return ret;
}

bool GstAppsrcWarp::CopyToGstBuffer(const GstMapInfo &info)
{
    guint8 *data = info.data;
    int32_t size = static_cast<int32_t>(info.size);
    while (size > 0) {
        std::shared_ptr<AppsrcMemWarp> appSrcMem = filledBuffers_.front();
        CHECK_AND_BREAK_LOG(appSrcMem != nullptr && appSrcMem->mem != nullptr
            && appSrcMem->mem->GetBase() != nullptr
            && (appSrcMem->size - appSrcMem->offset) > 0,
            "get mem is nullptr");
        int32_t lastSize = appSrcMem->size - appSrcMem->offset;
        int32_t copySize = std::min(lastSize, size);
        CHECK_AND_BREAK_LOG(memcpy_s(data, size, appSrcMem->mem->GetBase() + appSrcMem->offset, copySize) == EOK,
            "get mem is nullptr");
        if (lastSize <= size) {
            filledBuffers_.pop();
            emptyBuffers_.push(appSrcMem);
            fillCond_.notify_all();
        } else {
            appSrcMem->offset += copySize;
        }
        data = data + copySize;
        size -= copySize;
    }
    if (size != 0) {
        return false;
    }
    return true;
}

void GstAppsrcWarp::OnError(int32_t errorCode)
{
    PlayerErrorType errorType = PLAYER_ERROR_UNKNOWN;
    std::shared_ptr<IPlayerEngineObs> tempObs = obs_.lock();
    if (tempObs != nullptr) {
        tempObs->OnError(errorType, errorCode);
    }
}

void GstAppsrcWarp::PushData(const GstBuffer *buffer) const
{
    int32_t ret = GST_FLOW_OK;
    if (appSrc_ != nullptr) {
        g_signal_emit_by_name(appSrc_, "push-buffer", buffer, &ret);
    }
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