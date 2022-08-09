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

#include "player_track_parse.h"
#include "media_log.h"
#include "media_errors.h"
#include "av_common.h"
#include "gst_utils.h"
#include "gst_meta_parser.h"
#include "player.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "PlayerTrackParse"};
}

namespace OHOS {
namespace Media {
static const std::unordered_map<std::string_view, std::string_view> INNER_KEY_TO_PLAYER_KEY = {
    { INNER_META_KEY_BITRATE, PlayerKeys::PLAYER_BITRATE },
    { INNER_META_KEY_CHANNEL_COUNT, PlayerKeys::PLAYER_CHANNELS },
    { INNER_META_KEY_FRAMERATE, PlayerKeys::PLAYER_FRAMERATE },
    { INNER_META_KEY_VIDEO_HEIGHT, PlayerKeys::PLAYER_HEIGHT },
    { INNER_META_KEY_LANGUAGE, PlayerKeys::PLAYER_LANGUGAE },
    { INNER_META_KEY_MIME_TYPE, PlayerKeys::PLAYER_MIME },
    { INNER_META_KEY_SAMPLE_RATE, PlayerKeys::PLAYER_SAMPLE_RATE },
    { INNER_META_KEY_TRACK_INDEX, PlayerKeys::PLAYER_TRACK_INDEX },
    { INNER_META_KEY_TRACK_TYPE, PlayerKeys::PLAYER_TRACK_TYPE },
    { INNER_META_KEY_VIDEO_WIDTH, PlayerKeys::PLAYER_WIDTH },
};

std::shared_ptr<PlayerTrackParse> PlayerTrackParse::Create()
{
    std::shared_ptr<PlayerTrackParse> trackInfo = std::make_shared<PlayerTrackParse>();
    return trackInfo;
}

PlayerTrackParse::PlayerTrackParse()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

PlayerTrackParse::~PlayerTrackParse()
{
    MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t PlayerTrackParse::GetVideoTrackInfo(std::vector<Format> &videoTrack)
{
    int32_t trackType;
    for (auto &[pad, innerMeta] : trackInfos_) {
        if (innerMeta.GetIntValue(INNER_META_KEY_TRACK_TYPE, trackType) && trackType == MediaType::MEDIA_TYPE_VID) {
            Format outMeta;
            ConvertToPlayerKeys(innerMeta, outMeta);
            videoTrack.emplace_back(outMeta);
        }
    }
    return MSERR_OK;
}

int32_t PlayerTrackParse::GetAudioTrackInfo(std::vector<Format> &audioTrack)
{
    int32_t trackType;
    for (auto &[pad, innerMeta] : trackInfos_) {
        if (innerMeta.GetIntValue(INNER_META_KEY_TRACK_TYPE, trackType) && trackType == MediaType::MEDIA_TYPE_AUD) {
            Format outMeta;
            ConvertToPlayerKeys(innerMeta, outMeta);
            audioTrack.emplace_back(outMeta);
        }
    }
    return MSERR_OK;
}

void PlayerTrackParse::ConvertToPlayerKeys(const Format &innerMeta, Format &outMeta) const
{
    for (const auto &[innerKey, playerKey] : INNER_KEY_TO_PLAYER_KEY) {
        if (!innerMeta.ContainKey(innerKey)) {
            continue;
        }

        std::string strVal;
        int32_t intVal;
        FormatDataType type = innerMeta.GetValueType(innerKey);
        switch (type) {
            case FORMAT_TYPE_STRING:
                innerMeta.GetStringValue(innerKey, strVal);
                outMeta.PutStringValue(std::string(playerKey), strVal);
                break;
            case FORMAT_TYPE_INT32:
                innerMeta.GetIntValue(innerKey, intVal);
                outMeta.PutIntValue(std::string(playerKey), intVal);
                break;
            default:
                break;
        }
    }
}

GstPadProbeReturn PlayerTrackParse::ProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer userdata)
{
    if (pad == nullptr || info ==  nullptr || userdata == nullptr) {
        MEDIA_LOGE("param is invalid");
        return GST_PAD_PROBE_OK;
    }

    auto playerTrackParse = reinterpret_cast<PlayerTrackParse *>(userdata);
    auto it = playerTrackParse->trackInfos_.find(pad);
    CHECK_AND_RETURN_RET_LOG(it != playerTrackParse->trackInfos_.end(), GST_PAD_PROBE_OK,
        "unrecognized pad %{public}s", PAD_NAME(pad));

    if (static_cast<unsigned int>(info->type) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        GstEvent *event = gst_pad_probe_info_get_event(info);
        CHECK_AND_RETURN_RET_LOG(event != nullptr, GST_PAD_PROBE_OK, "event is null");

        if (GST_EVENT_TYPE(event) == GST_EVENT_TAG) {
            GstTagList *tagList = nullptr;
            gst_event_parse_tag(event, &tagList);
            CHECK_AND_RETURN_RET_LOG(tagList != nullptr, GST_PAD_PROBE_OK, "caps is nullptr")
            MEDIA_LOGI("catch tags at pad %{public}s", PAD_NAME(pad));
            GstMetaParser::ParseTagList(*tagList, it->second);
        } else if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
            GstCaps *caps = nullptr;
            gst_event_parse_caps(event, &caps);
            CHECK_AND_RETURN_RET_LOG(caps != nullptr, GST_PAD_PROBE_OK, "caps is nullptr")
            MEDIA_LOGI("catch caps at pad %{public}s", PAD_NAME(pad));
            GstMetaParser::ParseStreamCaps(*caps, it->second);
            it->second.PutIntValue(INNER_META_KEY_TRACK_INDEX, playerTrackParse->trackcount_);
            playerTrackParse->trackcount_++;
        }
    }

    return GST_PAD_PROBE_OK;
}

void PlayerTrackParse::AddProbeToPad(GstPad *pad)
{
    gulong probeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, ProbeCallback, this, nullptr);
    if (probeId == 0) {
        MEDIA_LOGE("add probe for %{public}s's pad %{public}s failed",
            GST_ELEMENT_NAME(GST_PAD_PARENT(pad)), PAD_NAME(pad));
        return;
    }
    (void)padProbes_.emplace(pad, probeId);

    Format innerMeta;
    (void)trackInfos_.emplace(pad, innerMeta);
}

void PlayerTrackParse::OnPadAddedCb(const GstElement *element, GstPad *pad, gpointer userdata)
{
    if (element == nullptr || pad ==  nullptr || userdata == nullptr) {
        MEDIA_LOGE("param is nullptr");
        return;
    }

    auto playerTrackParse = reinterpret_cast<PlayerTrackParse *>(userdata);
    playerTrackParse->AddProbeToPad(pad);
}

void PlayerTrackParse::SetDemuxerElementFind(bool isFind)
{
    demuxerElementFind_ = isFind;
}

bool PlayerTrackParse::GetDemuxerElementFind() const
{
    return demuxerElementFind_;
}
}
}
