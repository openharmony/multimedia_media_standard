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

#ifndef AVMUXER_ENGINE_GST_IMPL_H
#define AVMUXER_ENGINE_GST_IMPL_H

#include <set>
#include <map>
#include <tuple>
#include "nocopyable.h"
#include "i_avmuxer_engine.h"
#include "gst_msg_processor.h"
#include "gst_shmem_wrap_allocator.h"
#include "avmuxer_util.h"

namespace OHOS {
namespace Media {
class AVMuxerEngineGstImpl :
    public IAVMuxerEngine,
    public std::enable_shared_from_this<AVMuxerEngineGstImpl>,
    public NoCopyable {
public:
    AVMuxerEngineGstImpl();
    ~AVMuxerEngineGstImpl();

    std::vector<std::string> GetAVMuxerFormatList() override;
    int32_t Init() override;
    int32_t SetOutput(int32_t fd, const std::string &format) override;
    int32_t SetLocation(float latitude, float longitude) override;
    int32_t SetRotation(int32_t rotation) override;
    int32_t AddTrack(const MediaDescription &trackDesc, int32_t &trackId) override;
    int32_t Start() override;
    int32_t WriteTrackSample(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo) override;
    int32_t Stop() override;
private:
    void SetParse(const std::string &mimeType);
    int32_t WriteData(std::shared_ptr<AVSharedMemory> sampleData, const TrackSampleInfo &sampleInfo, GstElement *src);
    int32_t SetupMsgProcessor();
    void OnNotifyMessage(const InnerMessage &msg);
    void Clear();

    GstElement *muxBin_ = nullptr;
    std::map<int32_t, TrackInfo> trackInfo_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool endFlag_ = false;
    bool errHappened_ = false;
    std::unique_ptr<GstMsgProcessor> msgProcessor_;
    uint32_t videoTrackNum_ = 0;
    uint32_t audioTrackNum_ = 0;
    std::string format_;
    bool isPlay_ = false;
    GstShMemWrapAllocator *allocator_;
};
}  // namespace Media
}  // namespace OHOS
#endif // AVMUXER_ENGINE_GST_IMPL_H