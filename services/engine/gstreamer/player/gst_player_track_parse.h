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

#ifndef GST_PLAYER_TRACK_PARSE_H
#define GST_PLAYER_TRACK_PARSE_H

#include <string>
#include <vector>
#include <gst/gst.h>
#include <gst/player/player.h>
#include "player.h"

namespace OHOS {
namespace Media {
class GstPlayerTrackParse {
public:
    static std::shared_ptr<GstPlayerTrackParse> Create();
    int32_t GetVideoTrackInfo(std::vector<Format> &videoTrack);
    int32_t GetAudioTrackInfo(std::vector<Format> &audioTrack);
    void SetDemuxerElementFind(bool isFind);
    bool GetDemuxerElementFind();
    static void OnPadAddedCb(const GstElement *element, const GstPad *pad, GstPlayerTrackParse *playerTrackInfo);
    GstPlayerTrackParse() {};
    ~GstPlayerTrackParse() {};

private:
    static void ParseStreamStruc(const GstPad *pad, GstPlayerTrackParse *playerTrackInfo);
    static void ParseSingleCapsStructure(const GstStructure *struc, const GstPad *pad,
        std::vector<std::unordered_map<GstPad *, std::unordered_map<std::string, std::string>>> &trackInfoVec,
        std::vector<std::string_view> expectedCapsFields, int32_t index);
    static int32_t GetStrucFromPad(const GstPad *pad, GstStructure **struc);
    static int32_t GetMimeFromStruc(const GstStructure *struc, std::string &mime);
    static void AddProbeToPad(const GstPad *pad, GstPlayerTrackParse *playerTrackInfo);
    static GstPadProbeReturn ProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata);
    static void ParseTagList(const GstPad *pad, const GstTagList *tagList, GstPlayerTrackParse *playerTrackInfo);
    static std::unordered_map<std::string, std::string> *FindTrackInfoMap(const GstPad *pad,
        std::vector<std::unordered_map<GstPad *, std::unordered_map<std::string, std::string>>> &trackInfoVec);
    static void ParseTag(const GstTagList *tagList, guint tagIndex, std::vector<std::string_view> expectedTagFields,
        std::unordered_map<std::string, std::string> *trackInfoMap);
    static std::string GetSerializedValue(const GValue *value);
    static void ParseTagAndSaveTrackInfo(const GstPad *pad, const GstTagList *tagList,
        const std::vector<std::string_view> &expectedTagFields,
        std::vector<std::unordered_map<GstPad *, std::unordered_map<std::string, std::string>>> &trackInfoVec);
    int32_t trackIndex_ = 0;
    bool demuxerElementFind_ = false;
    std::vector<std::unordered_map<GstPad *, std::unordered_map<std::string, std::string>>> videoTrackInfo_;
    std::vector<std::unordered_map<GstPad *, std::unordered_map<std::string, std::string>>> audioTrackInfo_;
    std::unordered_map<GstPad *, gulong> padProbes_;
};
}
}
#endif
