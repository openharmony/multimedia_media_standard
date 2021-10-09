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

#include "avmeta_elem_meta_collector.h"
#include <string_view>
#include <limits>
#include "avmetadatahelper.h"
#include "media_errors.h"
#include "media_log.h"
#include "gst_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetaElemCollector"};
}

namespace OHOS {
namespace Media {
struct KeyToXMap {
    std::string_view keyName;
    int32_t innerKey;
};

#define AVMETA_KEY_TO_X_MAP_ITEM(key, innerKey) { key, { #key, innerKey }}
static const std::unordered_map<int32_t, KeyToXMap> AVMETA_KEY_TO_X_MAP = {
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_ALBUM, INNER_META_KEY_ALBUM),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_ALBUM_ARTIST, INNER_META_KEY_ALBUM_ARTIST),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_ARTIST, INNER_META_KEY_ARTIST),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_AUTHOR, INNER_META_KEY_AUTHOR),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_COMPOSER, INNER_META_KEY_COMPOSER),
    /**
     * The most of gst plugins don't send the GST_TAG_DURATION, we obtain this
     * infomation from duration query.
     */
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_DURATION, INNER_META_KEY_BUTT),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_GENRE, INNER_META_KEY_GENRE),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_HAS_AUDIO, INNER_META_KEY_HAS_AUDIO),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_HAS_VIDEO, INNER_META_KEY_HAS_VIDEO),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_MIME_TYPE, INNER_META_KEY_MIME_TYPE),
    /**
     * The GST_TAG_TRACK_COUNT does not means the actual track count, we obtain
     * this information from pads count.
     */
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_NUM_TRACKS, INNER_META_KEY_BUTT),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_SAMPLE_RATE, INNER_META_KEY_SAMPLE_RATE),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_TITLE, INNER_META_KEY_TITLE),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_VIDEO_HEIGHT, INNER_META_KEY_VIDEO_HEIGHT),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_VIDEO_WIDTH, INNER_META_KEY_VIDEO_WIDTH),
};

struct AVMetaElemMetaCollector::TrackInfo {
    int32_t tracknumber;
    Metadata metadata;
};

std::unique_ptr<AVMetaElemMetaCollector> AVMetaElemMetaCollector::Create(AVMetaSourceType type, const MetaResCb &resCb)
{
    switch (type) {
        case AVMetaSourceType::TYPEFIND:
            return std::make_unique<TypeFindMetaCollector>(type, resCb);
        case AVMetaSourceType::DEMUXER:
            return std::make_unique<DemuxerMetaCollector>(type, resCb);
        case AVMetaSourceType::PARSER:
            return std::make_unique<ParserMetaCollector>(type, resCb);
        default:
            MEDIA_LOGE("unknown type: %{public}hhu", type);
            break;
    }

    return nullptr;
}

Metadata AVMetaElemMetaCollector::GetDefaultMeta()
{
    Metadata defaultMetas;

    for (auto &item : AVMETA_KEY_TO_X_MAP) {
        defaultMetas.SetMeta(item.first, "");
    }

    defaultMetas.SetMeta(AV_KEY_NUM_TRACKS, "0");

    return defaultMetas;
}

AVMetaElemMetaCollector::AVMetaElemMetaCollector(AVMetaSourceType type, const MetaResCb &resCb)
    : type_(type), resCb_(resCb)
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR ", type: %{public}hhu", FAKE_POINTER(this), type_);
}

AVMetaElemMetaCollector::~AVMetaElemMetaCollector()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));

    for (auto &[elem, signalId] : signalIds_) {
        g_signal_handler_disconnect(elem, signalId);
    }

    for (auto &[pad, probeId] : padProbes_) {
        gst_pad_remove_probe(pad, probeId);
    }
}

int32_t AVMetaElemMetaCollector::GetTrackCount()
{
    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    return static_cast<int32_t>(trackInfos_.size());
}

bool AVMetaElemMetaCollector::AddProbeToPadList(GList &list)
{
    for (GList *padNode = g_list_first(&list); padNode != nullptr; padNode = padNode->next) {
        if (padNode->data == nullptr) {
            continue;
        }

        GstPad *pad = reinterpret_cast<GstPad *>(padNode->data);
        if (!AddProbeToPad(*pad)) {
            return false;
        }
    }

    return true;
}

bool AVMetaElemMetaCollector::AddProbeToPad(GstPad &pad)
{
    gulong probeId = gst_pad_add_probe(&pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, ProbeCallback, this, nullptr);
    if (probeId == 0) {
        MEDIA_LOGE("add probe for %{public}s's pad %{public}s failed", PAD_PARENT_NAME(&pad), PAD_NAME(&pad));
        return false;
    }

    (void)padProbes_.emplace(&pad, probeId);

    std::unique_lock<std::mutex> lock(trackInfoMutex_);
    int32_t tracknumber = static_cast<int32_t>(trackInfos_.size());
    (void)trackInfos_.emplace(&pad, TrackInfo {tracknumber, Metadata {}});

    fileMeta_.SetMeta(AV_KEY_NUM_TRACKS, std::to_string(trackInfos_.size()));
    fileMetaUpdated_ = true;
    MEDIA_LOGD("add probe to pad %{public}s of %{public}s", PAD_NAME(&pad), PAD_PARENT_NAME(&pad));
    return true;
}

bool AVMetaElemMetaCollector::ConnectSignal(GstElement &elem, std::string_view signal, GCallback callback)
{
    gulong signalId = g_signal_connect(&elem, signal.data(), callback, this);
    if (signalId == 0) {
        MEDIA_LOGE("connect signal '%{public}s' to %{public}s failed", signal.data(), ELEM_NAME(&elem));
        return false;
    }

    (void)signalIds_.emplace(&elem, signalId);
    return true;
}

GstPadProbeReturn AVMetaElemMetaCollector::ProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata)
{
    if (pad == nullptr || info ==  nullptr || usrdata == nullptr) {
        MEDIA_LOGE("param is invalid");
        return GST_PAD_PROBE_OK;
    }

    auto collector = reinterpret_cast<AVMetaElemMetaCollector *>(usrdata);
    if (static_cast<unsigned int>(info->type) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        GstEvent *event = gst_pad_probe_info_get_event(info);
        CHECK_AND_RETURN_RET_LOG(event != nullptr, GST_PAD_PROBE_OK, "event is null");
        collector->OnEventProbe(*pad, *event);
    }

    return GST_PAD_PROBE_OK;
}

void AVMetaElemMetaCollector::ParseTagList(const GstTagList &tagList, TrackInfo &trackInfo)
{
    GstTagScope scope = gst_tag_list_get_scope(&tagList);
    MEDIA_LOGI("catch tag %{public}s event", scope == GST_TAG_SCOPE_GLOBAL ? "global" : "stream");

    if (scope == GST_TAG_SCOPE_GLOBAL) {
        if (globalTagCatched_) {
            return;
        }
        globalTagCatched_ = true;
    }

    Metadata innerMeta;
    GstMetaParser::ParseTagList(tagList, innerMeta);
    if (innerMeta.tbl_.empty()) {
        return;
    }

    if (scope == GST_TAG_SCOPE_GLOBAL) {
        ConvertToAVMeta(innerMeta, fileMeta_);
        ReportMeta(AVMETA_TRACK_NUMBER_FILE, fileMeta_);
    } else {
        ConvertToAVMeta(innerMeta, trackInfo.metadata);
        ReportMeta(trackInfo.tracknumber, trackInfo.metadata);
    }
}

void AVMetaElemMetaCollector::ParseCaps(const GstCaps &caps, TrackInfo &trackInfo)
{
    Metadata innerMeta;
    GstMetaParser::ParseStreamCaps(caps, innerMeta);
    ConvertToAVMeta(innerMeta, trackInfo.metadata);
    ReportMeta(trackInfo.tracknumber, trackInfo.metadata);
}

void AVMetaElemMetaCollector::OnEventProbe(GstPad &pad, GstEvent &event)
{
    auto it = trackInfos_.find(&pad);
    CHECK_AND_RETURN_LOG(it != trackInfos_.end(), "unrecognized pad %{public}s", PAD_NAME(&pad));

    if (GST_EVENT_TYPE(&event) == GST_EVENT_TAG) {
        QueryDuration(pad);

        GstTagList *tagList = nullptr;
        gst_event_parse_tag(&event, &tagList);
        CHECK_AND_RETURN_LOG(tagList != nullptr, "taglist is nullptr");
        MEDIA_LOGI("catch tags at pad %{public}s", PAD_NAME(&pad));
        ParseTagList(*tagList, it->second);
    }

    if (GST_EVENT_TYPE(&event) == GST_EVENT_CAPS) {
        GstCaps *caps = nullptr;
        gst_event_parse_caps(&event, &caps);
        CHECK_AND_RETURN_LOG(caps != nullptr, "caps is nullptr");
        MEDIA_LOGI("catch caps at pad %{public}s", PAD_NAME(&pad));
        ParseCaps(*caps, it->second);
    }
}

void AVMetaElemMetaCollector::QueryDuration(GstPad &pad)
{
    GstQuery *query = gst_query_new_duration(GST_FORMAT_TIME);
    CHECK_AND_RETURN_LOG(query != nullptr, "query is failed");

    gint64 streamDuration = 0;
    if (gst_pad_query(&pad, query)) {
        GstFormat format = GST_FORMAT_TIME;
        gst_query_parse_duration(query, &format, &streamDuration);
        if (!GST_CLOCK_TIME_IS_VALID(streamDuration)) {
            streamDuration = 0;
        }
    }
    gst_query_unref(query);

    if (duration_ < streamDuration) {
        duration_ = streamDuration;
        MEDIA_LOGI("update duration to %{public}" PRIi64 "", duration_);

        static const int32_t NASEC_PER_HALF_MILLISEC = 500000;
        static const int32_t NASEC_PER_MILLISEC = 1000000;

        int64_t milliSecond;
        if ((std::numeric_limits<int64_t>::max() - NASEC_PER_HALF_MILLISEC) < duration_) {
            milliSecond = duration_ / NASEC_PER_MILLISEC; // ns -> ms
        } else {
            milliSecond = (duration_ + NASEC_PER_HALF_MILLISEC) / NASEC_PER_MILLISEC; // ns -> ms, round up.
        }

        fileMeta_.SetMeta(AV_KEY_DURATION, std::to_string(milliSecond));
        fileMetaUpdated_ = true;
    }
}

void AVMetaElemMetaCollector::ReportMeta(int32_t trackId, const Metadata &metadata)
{
    if (resCb_ == nullptr) {
        return;
    }

    if (fileMetaUpdated_) {
        resCb_(AVMETA_TRACK_NUMBER_FILE, fileMeta_);
        fileMetaUpdated_ = false;
    }

    resCb_(trackId, metadata);
}

void AVMetaElemMetaCollector::ConvertToAVMeta(const Metadata &innerMeta, Metadata &avmeta) const
{
    for (const auto &[avKey, keyToXItem] : AVMETA_KEY_TO_X_MAP) {
        if (keyToXItem.innerKey == INNER_META_KEY_BUTT) {
            continue;
        }

        std::string value;
        if (innerMeta.TryGetMeta(keyToXItem.innerKey, value)) {
            avmeta.SetMeta(avKey, value);
        }
    }
}

/**
 * Detail Element Meta Collector Implementation Begin.
 */

void TypeFindMetaCollector::HaveTypeCallback(GstElement *elem, guint probability, GstCaps *caps, gpointer userdata)
{
    if (elem == nullptr || caps == nullptr || userdata == nullptr) {
        return;
    }

    MEDIA_LOGD("typefind %{public}s have type, probalibity = %{public}u", ELEM_NAME(elem), probability);

    TypeFindMetaCollector *collector = reinterpret_cast<TypeFindMetaCollector *>(userdata);
    collector->OnHaveType(*elem, *caps);
}

void TypeFindMetaCollector::OnHaveType(const GstElement &elem, const GstCaps &caps)
{
    Metadata meta;
    GstMetaParser::ParseFileMimeType(caps, meta);

    std::string mimeType;
    bool ret = meta.TryGetMeta(INNER_META_KEY_MIME_TYPE, mimeType);
    if (!ret) {
        return;
    }

    Metadata avmeta;
    avmeta.SetMeta(AV_KEY_MIME_TYPE, mimeType);
    ReportMeta(AVMETA_TRACK_NUMBER_FILE, avmeta);
}

void TypeFindMetaCollector::AddMetaSource(GstElement &elem)
{
    (void)ConnectSignal(elem, "have-type", G_CALLBACK(&TypeFindMetaCollector::HaveTypeCallback));
}

void DemuxerMetaCollector::PadAddedCallback(GstElement *elem, GstPad *pad, gpointer userdata)
{
    if (elem == nullptr || pad == nullptr || userdata == nullptr) {
        return;
    }

    auto collector = reinterpret_cast<DemuxerMetaCollector *>(userdata);
    collector->OnPadAdded(*elem, *pad);
}

void DemuxerMetaCollector::OnPadAdded(GstElement &src, GstPad &pad)
{
    MEDIA_LOGD("demuxer %{public}s sinkpad %{public}s added", ELEM_NAME(&src), PAD_NAME(&pad));
    (void)AddProbeToPad(pad);
}

void DemuxerMetaCollector::AddMetaSource(GstElement &elem)
{
    if (!AddProbeToPadList(*elem.srcpads)) {
        return;
    }

    (void)ConnectSignal(elem, "pad-added", G_CALLBACK(&DemuxerMetaCollector::PadAddedCallback));
}

void ParserMetaCollector::AddMetaSource(GstElement &elem)
{
    (void)AddProbeToPadList(*elem.srcpads);
}
}
}
