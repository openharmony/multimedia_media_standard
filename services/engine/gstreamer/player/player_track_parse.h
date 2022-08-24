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

#ifndef PLAYER_TRACK_PARSE_H
#define PLAYER_TRACK_PARSE_H

#include <mutex>
#include <vector>
#include <unordered_map>
#include <gst/gst.h>
#include <gst/player/player.h>
#include "format.h"

namespace OHOS {
namespace Media {
class PlayerTrackParse {
public:
    static std::shared_ptr<PlayerTrackParse> Create();
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack);
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack);
    void SetDemuxerElementFind(bool isFind);
    bool GetDemuxerElementFind() const;
    void SetUpDemuxerElementCb(GstElement &elem);
    void Stop();
    static void OnPadAddedCb(const GstElement *element, GstPad *pad, gpointer userdata);
    PlayerTrackParse();
    ~PlayerTrackParse();

private:
    GstPadProbeReturn GetTrackParse(GstPad *pad, GstPadProbeInfo *info);
    void ConvertToPlayerKeys(const Format &innerMeta, Format &outMeta) const;
    void AddProbeToPad(GstPad *pad);
    static GstPadProbeReturn ProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata);
    bool demuxerElementFind_ = false;
    std::unordered_map<GstPad *, Format> trackInfos_;
    int32_t trackcount_ = 0;
    struct SignalInfo {
        GstElement *element;
        gulong signalId;
    };
    std::vector<SignalInfo> signalIds_;
    struct PadInfo {
        GstPad *pad;
        gulong probeId;
    };
    std::vector<PadInfo> padProbes_;
    std::mutex signalIdMutex_;
    std::mutex padProbeMutex_;
    std::mutex trackInfoMutex_;
};
}
}
#endif
