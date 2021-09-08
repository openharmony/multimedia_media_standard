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

#ifndef GST_APPSRC_WARP_H_
#define GST_APPSRC_WARP_H_

#include <gst/gst.h>
#include "task_queue.h"
#include "media_data_source.h"
#include "gst/app/gstappsrc.h"
#include "i_player_engine.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class GstAppsrcWarp {
public:
    static std::shared_ptr<GstAppsrcWarp> Create(const std::shared_ptr<IMediaDataSource> &dataSrc);
    GstAppsrcWarp(const std::shared_ptr<IMediaDataSource> &dataSrc, const int64_t size);
    ~GstAppsrcWarp();
    DISALLOW_COPY_AND_MOVE(GstAppsrcWarp);
    int32_t SetAppsrc(GstElement *appSrc);
    int32_t SetErrorCallback(const std::weak_ptr<IPlayerEngineObs> &obs);
    bool NoSeek() const;

private:
    void SetCallBackForAppSrc();
    void ClearAppsrc();
    static void NeedData(const GstElement *appSrc, uint32_t size, gpointer self);
    static gboolean SeekData(const GstElement *appSrc, uint64_t seekPos, gpointer self);
    void NeedDataInner(uint32_t size);
    gboolean SeekDataInner(uint64_t seekPos);
    void ReadAndGetMem();
    void AnalyzeSize(int32_t size);
    int32_t GetAndPushMem(int32_t size);
    void OnError(int32_t errorCode);
    void PushData(void *buffer);
    void PushEos();
    std::shared_ptr<IMediaDataSource> dataSrc_ = nullptr;
    const int64_t size_;
    uint64_t curPos_ = 0;
    uint64_t targetPos_ = 0;
    std::mutex mutex_;
    GstElement *appSrc_ = nullptr;
    TaskQueue taskQue_;
    GstAppStreamType streamType_ = GST_APP_STREAM_TYPE_STREAM;
    std::weak_ptr<IPlayerEngineObs> obs_;
    std::vector<gulong> callbackIds_;
};
} // namespace Media
} // namespace OHOS
#endif /* GST_APPSRC_WARP_H_ */