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

#include "gst_player_track_parse.h"
#include "media_log.h"
#include "media_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "GstPlayerTrackParse"};
}

namespace OHOS {
namespace Media {
static const uint32_t MEDIA_TYPE_AUD = 0;
static const uint32_t MEDIA_TYPE_VID = 1;
static const std::unordered_map<std::string_view, std::string> TRACK_FORMAT_STRING_CHANGE = {
    {PlayerKeys::PLAYER_WIDTH, "width"}, {PlayerKeys::PLAYER_HEIGHT, "height"}, {PlayerKeys::PLAYER_MIME, "codec_mime"},
    {PlayerKeys::PLAYER_BITRATE, "bitrate"}, {PlayerKeys::PLAYER_FRAMERATE, "framerate"},
    {PlayerKeys::PLAYER_LANGUGAE, "language-code"}, {PlayerKeys::PLAYER_SAMPLE_RATE, "rate"},
    {PlayerKeys::PLAYER_CHANNELS, "channels"},
};
std::shared_ptr<GstPlayerTrackParse> GstPlayerTrackParse::Create()
{
    std::shared_ptr<GstPlayerTrackParse> trackInfo = std::make_shared<GstPlayerTrackParse>();
    return trackInfo;
}

int32_t GstPlayerTrackParse::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    int32_t value;
    for (auto iter = videoTrackInfo_.begin(); iter != videoTrackInfo_.end(); iter++) {
        Format trackInfo;
        auto trackInfoMap = iter->begin()->second;
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_MIME)) > 0) {
            std::string strValue = trackInfoMap.at(std::string(PlayerKeys::PLAYER_MIME));
            (void)trackInfo.PutStringValue(std::string(PlayerKeys::PLAYER_MIME), strValue);
        }
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_BITRATE)) > 0) {
            value = std::stoi(trackInfoMap.at(std::string(PlayerKeys::PLAYER_BITRATE)));
            (void)trackInfo.PutIntValue(std::string(PlayerKeys::PLAYER_BITRATE), value);
        }
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_WIDTH)) > 0) {
            value = std::stoi(trackInfoMap.at(std::string(PlayerKeys::PLAYER_WIDTH)));
            (void)trackInfo.PutIntValue(std::string(PlayerKeys::PLAYER_WIDTH), value);
        }
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_HEIGHT)) > 0) {
            value = std::stoi(trackInfoMap.at(std::string(PlayerKeys::PLAYER_HEIGHT)));
            (void)trackInfo.PutIntValue(std::string(PlayerKeys::PLAYER_HEIGHT), value);
        }
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_FRAMERATE)) > 0) {
            std::string framerate = trackInfoMap.at(std::string(PlayerKeys::PLAYER_FRAMERATE));
            std::string::size_type delimPos = framerate.find('/');
            if (delimPos == std::string::npos) {
                MEDIA_LOGE("unrecognizable framerate, %{public}s", framerate.c_str());
                return MSERR_UNKNOWN;
            }
            int32_t num = std::stoi(framerate.substr(0, delimPos));
            int32_t den = std::stoi(framerate.substr(delimPos + 1, framerate.length()));
            if (den != 0) {
                /* 100: float to int ratio */
                value = (static_cast<float>(num) / den) * 100;
            } else {
                value = num;
            }

            (void)trackInfo.PutIntValue(std::string(PlayerKeys::PLAYER_FRAMERATE), value);
        }
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_TRACK_INDEX)) > 0) {
            value = std::stoi(trackInfoMap.at(std::string(PlayerKeys::PLAYER_TRACK_INDEX)));
            (void)trackInfo.PutIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), value);
        }
        (void)trackInfo.PutIntValue(std::string(PlayerKeys::PLAYER_TRACK_TYPE), MEDIA_TYPE_VID);
        videoTrack.push_back(trackInfo);
    }
    return MSERR_OK;
}

int32_t GstPlayerTrackParse::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    for (auto iter = audioTrackInfo_.begin(); iter != audioTrackInfo_.end(); iter++) {
        Format trackInfo;
        auto trackInfoMap = iter->begin()->second;
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_MIME)) > 0) {
            std::string value = trackInfoMap.at(std::string(PlayerKeys::PLAYER_MIME));
            (void)trackInfo.PutStringValue(std::string(PlayerKeys::PLAYER_MIME), value);
        }
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_BITRATE)) > 0) {
            int32_t value = std::stoi(trackInfoMap.at(std::string(PlayerKeys::PLAYER_BITRATE)));
            (void)trackInfo.PutIntValue(std::string(PlayerKeys::PLAYER_BITRATE), value);
        }
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_SAMPLE_RATE)) > 0) {
            int32_t value = std::stoi(trackInfoMap.at(std::string(PlayerKeys::PLAYER_SAMPLE_RATE)));
            (void)trackInfo.PutIntValue(std::string(PlayerKeys::PLAYER_SAMPLE_RATE), value);
        }
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_CHANNELS)) > 0) {
            int32_t value = std::stoi(trackInfoMap.at(std::string(PlayerKeys::PLAYER_CHANNELS)));
            (void)trackInfo.PutIntValue(std::string(PlayerKeys::PLAYER_CHANNELS), value);
        }
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_LANGUGAE)) > 0) {
            std::string value = trackInfoMap.at(std::string(PlayerKeys::PLAYER_LANGUGAE));
            (void)trackInfo.PutStringValue(std::string(PlayerKeys::PLAYER_LANGUGAE), value);
        }
        if (trackInfoMap.count(std::string(PlayerKeys::PLAYER_TRACK_INDEX)) > 0) {
            int32_t value = std::stoi(trackInfoMap.at(std::string(PlayerKeys::PLAYER_TRACK_INDEX)));
            (void)trackInfo.PutIntValue(std::string(PlayerKeys::PLAYER_TRACK_INDEX), value);
        }
        (void)trackInfo.PutIntValue(std::string(PlayerKeys::PLAYER_TRACK_TYPE), MEDIA_TYPE_AUD);
        audioTrack.push_back(trackInfo);
    }
    return MSERR_OK;
}

std::string GstPlayerTrackParse::GetSerializedValue(const GValue *value)
{
    if (G_VALUE_HOLDS_STRING(value)) {
        const gchar *str = g_value_get_string(value);
        CHECK_AND_RETURN_RET_LOG(str != nullptr, "", "gvalue has no null value");
        MEDIA_LOGI("value: %{public}s", str);
        return std::string(str);
    }

    gchar *str = gst_value_serialize(value);
    CHECK_AND_RETURN_RET_LOG(str != nullptr, "", "serialize value failed");
    std::string serializedValue = str;
    g_free(str);

    return serializedValue;
}

void GstPlayerTrackParse::ParseSingleCapsStructure(const GstStructure *struc, const GstPad *pad,
    std::vector<std::unordered_map<GstPad *, std::unordered_map<std::string, std::string>>> &trackInfoVec,
    std::vector<std::string_view> expectedCapsFields, int32_t index)
{
    std::unordered_map<std::string, std::string> trackMap;
    std::string mime = gst_structure_get_name(struc);

    (void)trackMap.insert(
        std::pair<std::string, std::string>(std::string(PlayerKeys::PLAYER_TRACK_INDEX), std::to_string(index)));
    (void)trackMap.insert(std::pair<std::string, std::string>(std::string(PlayerKeys::PLAYER_MIME), mime));
    for (auto &field : expectedCapsFields) {
        auto iterStr = TRACK_FORMAT_STRING_CHANGE.find(field);
        if (iterStr == TRACK_FORMAT_STRING_CHANGE.end()) {
            MEDIA_LOGE("not match expected cap, %{public}s", field.data());
            continue;
        }
        const GValue *val = gst_structure_get_value(struc, iterStr->second.c_str());
        if (val == nullptr) {
            MEDIA_LOGE("get %{public}s filed data value error", iterStr->second.c_str());
            continue;
        }

        std::string serializedValue = GetSerializedValue(val);
        MEDIA_LOGD("field %{public}s val %{public}s", iterStr->second.c_str(), serializedValue.c_str());
        trackMap.insert(std::pair<std::string, std::string>(std::string(field), serializedValue));
    }
    std::unordered_map<GstPad *, std::unordered_map<std::string, std::string>> padMap;
    (void)padMap.insert(std::pair<GstPad *, std::unordered_map<std::string, std::string>>((GstPad *)pad, trackMap));
    trackInfoVec.push_back(padMap);
}

int32_t GstPlayerTrackParse::GetMimeFromStruc(const GstStructure *struc, std::string &mime)
{
    const gchar *name = gst_structure_get_name(struc);
    if (name == nullptr) {
        MEDIA_LOGE("gst_structure_get_name fail");
        return MSERR_UNKNOWN;
    }
    std::string mimeType = name;
    std::string::size_type delimPos = mimeType.find('/');
    if (delimPos == std::string::npos) {
        MEDIA_LOGE("unrecognizable mimetype, %{public}s", mimeType.c_str());
        return MSERR_UNKNOWN;
    }
    MEDIA_LOGI("parse mimetype %{public}s's caps", mimeType.c_str());
    mime = mimeType.substr(0, delimPos);
    return MSERR_OK;
}

void GstPlayerTrackParse::ParseStreamStruc(const GstPad *pad, GstPlayerTrackParse *playerTrackInfo)
{
    std::string mime;
    GstStructure *structure;

    int32_t retVal = GetStrucFromPad(pad, &structure);
    if (retVal != MSERR_OK) {
        return;
    }
    retVal = GetMimeFromStruc(structure, mime);
    if (retVal != MSERR_OK) {
        return;
    }
    if (mime.compare("video") == 0) {
        std::vector<std::string_view> expectedCapsFields = {
            PlayerKeys::PLAYER_WIDTH, PlayerKeys::PLAYER_HEIGHT, PlayerKeys::PLAYER_FRAMERATE
        };
        ParseSingleCapsStructure(structure, pad, playerTrackInfo->videoTrackInfo_,
            expectedCapsFields, playerTrackInfo->trackIndex_);
        playerTrackInfo->trackIndex_++;
    } else if (mime.compare("audio") == 0) {
        std::vector<std::string_view> expectedCapsFields = {
            PlayerKeys::PLAYER_SAMPLE_RATE, PlayerKeys::PLAYER_CHANNELS
        };
        ParseSingleCapsStructure(structure, pad, playerTrackInfo->audioTrackInfo_,
            expectedCapsFields, playerTrackInfo->trackIndex_);
        playerTrackInfo->trackIndex_++;
    } else {
        MEDIA_LOGE("parse streamType %{public}s's not match", mime.c_str());
        return;
    }
}

int32_t GstPlayerTrackParse::GetStrucFromPad(const GstPad *pad, GstStructure **struc)
{
    GstCaps *caps = gst_pad_get_current_caps(const_cast<GstPad *>(pad));
    if (caps == nullptr) {
        MEDIA_LOGE("gst_pad_get_current_caps return nullptr");
        return MSERR_UNKNOWN;
    }

    MEDIA_LOGD("add probe for %{public}s's pad %{public}s",
        GST_ELEMENT_NAME(GST_PAD_PARENT(pad)), GST_PAD_NAME(pad));

    *struc = gst_caps_get_structure(caps, 0);
    if (*struc == nullptr) {
        MEDIA_LOGE("gst_caps_get_structure return nullptr");
        return MSERR_UNKNOWN;
    }
    return MSERR_OK;
}

std::unordered_map<std::string, std::string> *GstPlayerTrackParse::FindTrackInfoMap(const GstPad *pad,
    std::vector<std::unordered_map<GstPad *, std::unordered_map<std::string, std::string>>> &trackInfoVec)
{
    for (auto iter = trackInfoVec.begin(); iter != trackInfoVec.end(); iter++) {
        auto padToMapIt = iter->find(const_cast<GstPad *>(pad));
        if (padToMapIt == iter->end()) {
            return nullptr;
        }
        return &(padToMapIt->second);
    }

    return nullptr;
}

void GstPlayerTrackParse::ParseTag(const GstTagList *tagList, guint tagIndex,
    std::vector<std::string_view> expectedTagFields,
    std::unordered_map<std::string, std::string> *trackInfoMap)
{
    const gchar *tag = gst_tag_list_nth_tag_name(tagList, tagIndex);
    if (tag == nullptr) {
        return;
    }

    MEDIA_LOGD("visit tag: %{public}s", tag);
    for (auto iter = expectedTagFields.begin(); iter != expectedTagFields.end(); iter++) {
        auto iterStr = TRACK_FORMAT_STRING_CHANGE.find(*iter);
        if (iterStr == TRACK_FORMAT_STRING_CHANGE.end()) {
            MEDIA_LOGE("not match expected tag, %{public}s", iter->data());
            continue;
        }
        if (iterStr->second.compare(tag) == 0) {
            const GValue *value = gst_tag_list_get_value_index(tagList, tag, 0);
            if (value == nullptr) {
                MEDIA_LOGE("gst_tag_list_get_value_index get tag %{public}s fail", tag);
                return;
            }
            std::string serializedValue = GetSerializedValue(value);
            trackInfoMap->insert(std::pair<std::string, std::string>(std::string(*iter), serializedValue));
            MEDIA_LOGD("trackInfoMap: %{public}s, %{public}s", iter->data(), serializedValue.c_str());
            break;
        }
    }
}

void GstPlayerTrackParse::ParseTagAndSaveTrackInfo(const GstPad *pad, const GstTagList *tagList,
    std::vector<std::string_view> expectedTagFields,
    std::vector<std::unordered_map<GstPad *, std::unordered_map<std::string, std::string>>> &trackInfoVec)
{
    std::unordered_map<std::string, std::string> *trackInfoMap;
    gint tagCnt = gst_tag_list_n_tags(tagList);
    if (tagCnt < 0) {
        return;
    }

    trackInfoMap = FindTrackInfoMap(pad, trackInfoVec);
    if (trackInfoMap == nullptr) {
        MEDIA_LOGE("not find trackInfoMap at pad %{public}s", GST_PAD_NAME(pad));
        return;
    }

    for (guint tagIndex = 0; tagIndex < static_cast<guint>(tagCnt); tagIndex++) {
        ParseTag(tagList, tagIndex, expectedTagFields, trackInfoMap);
    }
}

void GstPlayerTrackParse::ParseTagList(const GstPad *pad, const GstTagList *tagList,
                                       GstPlayerTrackParse *playerTrackInfo)
{
    std::vector<std::string_view> expectedTagFields;
    GstStructure *structure;
    std::string mime;

    int32_t retVal = GetStrucFromPad(pad, &structure);
    if (retVal != MSERR_OK) {
        return;
    }
    retVal = GetMimeFromStruc(structure, mime);
    if (retVal != MSERR_OK) {
        return;
    }
    if (mime.compare("video") == 0) {
        expectedTagFields = {PlayerKeys::PLAYER_BITRATE};
        ParseTagAndSaveTrackInfo(pad, tagList, expectedTagFields, playerTrackInfo->videoTrackInfo_);
    } else if (mime.compare("audio") == 0) {
        expectedTagFields = {PlayerKeys::PLAYER_BITRATE, PlayerKeys::PLAYER_LANGUGAE};
        ParseTagAndSaveTrackInfo(pad, tagList, expectedTagFields, playerTrackInfo->audioTrackInfo_);
    } else {
        MEDIA_LOGE("parse streamType %{public}s's not match", mime.c_str());
        return;
    }
}

GstPadProbeReturn GstPlayerTrackParse::ProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata)
{
    if (pad == nullptr || info ==  nullptr || usrdata == nullptr) {
        MEDIA_LOGE("param is invalid");
        return GST_PAD_PROBE_OK;
    }

    if (static_cast<unsigned int>(info->type) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        GstEvent *event = gst_pad_probe_info_get_event(info);
        CHECK_AND_RETURN_RET_LOG(event != nullptr, GST_PAD_PROBE_OK, "event is null");
        if (GST_EVENT_TYPE(event) == GST_EVENT_TAG) {
            GstTagList *tagList = nullptr;
            gst_event_parse_tag(event, &tagList);
            if (tagList == nullptr) {
                MEDIA_LOGE("taglist is invalid");
                return GST_PAD_PROBE_OK;
            }
            MEDIA_LOGI("catch tags at pad %{public}s", GST_PAD_NAME(pad));
            ParseTagList(pad, tagList, reinterpret_cast<GstPlayerTrackParse *>(usrdata));
        }
    }

    return GST_PAD_PROBE_OK;
}

void GstPlayerTrackParse::AddProbeToPad(const GstPad *pad, GstPlayerTrackParse *playerTrackInfo)
{
    gulong probeId = gst_pad_add_probe(const_cast<GstPad *>(pad), GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
                                       ProbeCallback, playerTrackInfo, nullptr);
    if (probeId == 0) {
        MEDIA_LOGE("add probe for %{public}s's pad %{public}s failed",
            GST_ELEMENT_NAME(GST_PAD_PARENT(pad)), GST_PAD_NAME(pad));
        return;
    }
    (void)playerTrackInfo->padProbes_.insert(std::pair<GstPad *, gulong>((GstPad *)pad, probeId));
}

void GstPlayerTrackParse::OnPadAddedCb(const GstElement *element,
    const GstPad *pad, GstPlayerTrackParse *playerTrackInfo)
{
    CHECK_AND_RETURN_LOG(element != nullptr, "element is null");
    CHECK_AND_RETURN_LOG(playerTrackInfo != nullptr, "playerTrackInfo is null");
    CHECK_AND_RETURN_LOG(pad != nullptr, "pad is null");

    ParseStreamStruc(pad, playerTrackInfo);
    AddProbeToPad(pad, playerTrackInfo);
}

void GstPlayerTrackParse::SetDemuxerElementFind(bool isFind)
{
    demuxerElementFind_ = isFind;
}

bool GstPlayerTrackParse::GetDemuxerElementFind()
{
    return demuxerElementFind_;
}
}
}
