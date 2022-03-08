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

#ifndef AVMETA_FRAME_EXTRACTOR_H
#define AVMETA_FRAME_EXTRACTOR_H

#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "avmetadatahelper_engine_gst_impl.h"
#include "avmeta_frame_converter.h"
#include "nocopyable.h"

namespace OHOS {
namespace Media {
class AVMetaFrameExtractor : public NoCopyable {
public:
    AVMetaFrameExtractor();
    ~AVMetaFrameExtractor();

    int32_t Init(const std::shared_ptr<IPlayBinCtrler> &playbin, GstElement &vidAppSink);
    std::shared_ptr<AVSharedMemory> ExtractFrame(int64_t timeUs, int32_t option, const OutputConfiguration &param);
    void Reset();
    void NotifyPlayBinMsg(const PlayBinMessage &msg);

private:
    int32_t SetupVideoSink();
    int32_t StartExtract(int32_t numFrames, int64_t timeUs, int32_t option, const OutputConfiguration &param);
    std::vector<std::shared_ptr<AVSharedMemory>> ExtractInternel();
    void StopExtract();
    void ClearCache();

    static GstFlowReturn OnNewPrerollArrived(GstElement *sink, AVMetaFrameExtractor *thiz);

    std::shared_ptr<IPlayBinCtrler> playbin_;
    GstElement *vidAppSink_ = nullptr;
    std::queue<std::pair<GstBuffer *, GstCaps *>> originalFrames_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool seekDone_ = false;
    bool startExtracting_ = false;
    std::unique_ptr<AVMetaFrameConverter> frameConverter_;
    std::vector<gulong> signalIds_;
};
} // namespace Media
} // namespace OHOS
#endif // AVMETA_FRAME_EXTRACTOR_H
