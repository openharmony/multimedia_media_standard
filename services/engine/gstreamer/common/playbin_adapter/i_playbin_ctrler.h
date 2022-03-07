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
#include "playbin_msg_define.h"
#include "playbin_sink_provider.h"

namespace OHOS {
namespace Media {
class IPlayBinCtrler {
public:
    enum class PlayBinKind : uint8_t {
        PLAYBIN2,
    };

    enum PlayBinRenderMode : uint8_t {
        DEFAULT_RENDER = 0,
        NATIVE_STREAM = 1 << 0,
        DISABLE_TEXT = 1 << 1,
    };

    enum PlayBinSeekMode : uint8_t {
        NEXT_SYNC,
        PREV_SYNC,
        CLOSET_SYNC,
        CLOSET,
    };

    struct PlayBinCreateParam {
        PlayBinRenderMode renderMode;
        PlayBinMsgNotifier notifier;
        std::shared_ptr<PlayBinSinkProvider> sinkProvider;
    };

    virtual ~IPlayBinCtrler() = default;

    static std::shared_ptr<IPlayBinCtrler> Create(PlayBinKind kind, const PlayBinCreateParam &createParam);

    virtual int32_t SetSource(const std::string &url) = 0;
    virtual int32_t Prepare() = 0; // sync
    virtual int32_t PrepareAsync() = 0; // async
    virtual int32_t Play() = 0; // async
    virtual int32_t Pause() = 0; // async
    virtual int32_t Seek(int64_t timeUs, int32_t seekOption) = 0; // async
    virtual int32_t Stop() = 0; // async
    virtual int64_t GetDuration() = 0; // usec

    using ElemSetupListener = std::function<void(GstElement &elem)>;
    virtual void SetElemSetupListener(ElemSetupListener listener) = 0;
};
} // namespace Media
} // namespace OHOS
#endif // I_PLAYBIN_CTRLER_H