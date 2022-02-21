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

#ifndef AVMETA_META_COLLECTOR_H
#define AVMETA_META_COLLECTOR_H

#include <unordered_map>
#include <set>
#include <condition_variable>
#include <mutex>
#include <nocopyable.h>
#include <gst/gst.h>
#include "avmeta_elem_meta_collector.h"
#include "avmeta_buffer_blocker.h"

namespace OHOS {
namespace Media {
class AVMetaMetaCollector : public NoCopyable {
public:
    AVMetaMetaCollector();
    ~AVMetaMetaCollector();

    void Start();
    void AddMetaSource(GstElement &source);
    void Stop(bool unlock = false);
    std::unordered_map<int32_t, std::string> GetMetadata();
    std::string GetMetadata(int32_t key);
    std::shared_ptr<AVSharedMemory> FetchArtPicture();

private:
    uint8_t ProbeElemType(GstElement &source);
    void AddElemCollector(GstElement &source, uint8_t elemType);
    void AddElemBlocker(GstElement &source, uint8_t elemType);
    void UpdateElemBlocker(GstElement &source, uint8_t elemType);
    void UpdataMeta(const Metadata &metadata);
    bool CheckCollectCompleted() const;
    void AdjustMimeType();
    void StopBlocker(bool unlock);
    static void PadAdded(GstElement *elem, GstPad *pad, gpointer userdata);

    std::mutex mutex_;
    std::condition_variable cond_;
    std::vector<std::unique_ptr<AVMetaElemMetaCollector>> elemCollectors_;
    std::unordered_map<uint8_t, uint8_t> hasSrcType_;
    Metadata allMeta_;
    bool stopCollecting_ = false;

    class MultiQueueCutOut;
    std::unique_ptr<MultiQueueCutOut> mqCutOut_;

    using BufferBlockerVec = std::vector<std::shared_ptr<AVMetaBufferBlocker>>;
    std::unordered_map<uint8_t, BufferBlockerVec> blockers_;
};
}
}

#endif
