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

#ifndef PLAYBIN_SINK_PROVIDER_H
#define PLAYBIN_SINK_PROVIDER_H

#include <gst/gst.h>
#include "playbin_msg_define.h"

namespace OHOS {
namespace Media {
class PlayBinSinkProvider {
public:
    virtual ~PlayBinSinkProvider() = default;

    using SinkPtr = GstElement *;
    virtual SinkPtr CreateVideoSink() = 0;
    virtual SinkPtr CreateAudioSink() = 0;
    virtual void SetAppInfo(int32_t uid, int32_t pid)
    {
        (void)uid;
        (void)pid;
    }
    virtual void SetVideoScaleType(const uint32_t videoScaleType)
    {
        (void)videoScaleType;
    }
    virtual void SetMsgNotifier(PlayBinMsgNotifier notifier)
    {
        (void)notifier;
    }
    virtual SinkPtr GetVideoSink()
    {
        return nullptr;
    }
};
} // namespace Media
} // namespace OHOS
#endif