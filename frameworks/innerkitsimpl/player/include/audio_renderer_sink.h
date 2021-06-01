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

#ifndef AUDIO_RENDERER_SINK_H
#define AUDIO_RENDERER_SINK_H

#include "audio_manager.h"
#include "audio_types.h"
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiosink.h>
#include <gst/gst.h>

namespace OHOS {
namespace Media {
/**
 * AudioRendererSink:
 */
class AudioRendererSink {
public:
    AudioRendererSink();
    virtual ~AudioRendererSink() = default;
    bool Prepare(GstCaps &Caps);
    bool Stop(void);
    int32_t Write(gpointer, guint);
    int32_t SetVolume(float volume);
    void Close();
    bool Open(void);
    void Reset(void);
    bool ParseSpec(GstAudioRingBufferSpec &Spec);
    bool ParseCaps(GstCaps &Caps, GstAudioRingBufferSpec &Spec);

    AudioManager *audioManager_;
    AudioAdapter *audioAdapter_;
    AudioRender *audioRender_;
    AudioFormat audioFormat_;
    uint32_t sampleRate;
    uint32_t channelCount;
    FILE *pfd;
    bool enableDebug;
};
} // namespace Media
} // namespace OHOS
#endif  // AUDIO_RENDERER_SINK_H
