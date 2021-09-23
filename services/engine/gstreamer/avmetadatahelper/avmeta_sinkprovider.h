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

#ifndef AVMETA_SINKPROVIDER_H
#define AVMETA_SINKPROVIDER_H

#include <cstdint>
#include <memory>
#include <nocopyable.h>
#include <gst/gst.h>
#include "playbin_sink_provider.h"
#include "frame_callback.h"

namespace OHOS {
namespace Media {
class AVMetaSinkProvider : public PlayBinSinkProvider {
public:
    explicit AVMetaSinkProvider(int32_t usage);
    ~AVMetaSinkProvider();

    SinkPtr CreateAudioSink() override;
    SinkPtr CreateVideoSink() override;
    void SetFrameCallback(const std::shared_ptr<FrameCallback> &callback);

    DISALLOW_COPY_AND_MOVE(AVMetaSinkProvider);

private:
    int32_t usage_;
    GstElement *audSink_ = nullptr;
    GstElement *vidSink_ = nullptr;
    std::shared_ptr<FrameCallback> callback_;
};
}
}

#endif
