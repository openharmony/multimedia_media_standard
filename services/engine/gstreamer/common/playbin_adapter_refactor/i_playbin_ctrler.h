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

#ifndef I_PLAYBIN_CTRLER_H
#define I_PLAYBIN_CTRLER_H

#include <memory>
#include <string>
#include <unordered_map>
#include "nocopyable.h"
#include "gst_msg_converter.h"
#include "media_data_source.h"
#include "playbin_msg_define.h"
#include "playbin_sink_provider.h"
#include "playbin_source.h"
#include "task_queue.h"
#include "uri_helper.h"

namespace OHOS {
namespace Media {
namespace PlayBin {
class IPlayBinCtrler {
public:
    enum RenderMode : uint8_t {
        // use the playbin default render pipeline
        DEFAULT_RENDER = 0,
        // no audio software-transform pipeline after audio decoder
        NATIVE_AUDIO_STREAM = 1 << 0,
        // no video software-transform pipeline after video decoder
        NATIVE_VIDEO_STREAM = 1 << 1,
        // no video software-transform pipeline after video decoder if decoder is hardware decoder
        HARDWARE_NATIVE_VIDEO_STREAM = 1 << 2,
        // no subtitle process pipeline
        DISABLE_TEXT = 1 << 3,
    };

    enum SeekMode : uint8_t {
        NEXT_SYNC,
        PREV_SYNC,
        CLOSET_SYNC,
        CLOSET,
    };

    struct CreateParam {
        RenderMode renderMode = RenderMode::DEFAULT_RENDER;
        InnerMsgNotifier notifier;
        std::shared_ptr<SinkProvider> sinkProvider;
        std::shared_ptr<IGstMsgConverter> msgConverter;
        std::shared_ptr<TaskQueue> workLooper;
    };

    virtual ~IPlayBinCtrler() = default;

    static std::shared_ptr<IPlayBinCtrler> Create(const CreateParam &createParam);

    virtual int32_t SetSource(const std::shared_ptr<SourceBase> &source) = 0; // sync
    virtual int32_t Prepare() = 0; // async
    virtual int32_t Play() = 0; // async
    virtual int32_t Pause() = 0; // async
    virtual int32_t Seek(int64_t timeUs, SeekMode seekOption) = 0; // async
    virtual int32_t SetSpeed(double speed) = 0; // async
    virtual int32_t Stop() = 0; // async
    virtual int32_t Reset() = 0; // sync
    virtual int64_t GetDuration() = 0; // sync usec
    virtual int64_t GetPosition() = 0; // sync usec
    virtual double GetSpeed() = 0; // sync
};
} // namespace PlayBin
} // namespace Media
} // namespace OHOS
#endif // I_PLAYBIN_CTRLER_H