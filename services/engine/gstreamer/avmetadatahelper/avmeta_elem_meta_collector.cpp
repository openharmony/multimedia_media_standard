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
#include "avsharedmemorybase.h"
#include "av_common.h"
#include "gst_meta_parser.h"
#include "gst_utils.h"
#include "media_errors.h"
#include "media_log.h"
#include "securec.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetaElemCollector"};
}

namespace OHOS {
namespace Media {
#define AVMETA_KEY_TO_X_MAP_ITEM(key, innerKey) { key, innerKey }

static const std::unordered_map<int32_t, std::string_view> AVMETA_KEY_TO_X_MAP = {
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_ALBUM, INNER_META_KEY_ALBUM),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_ALBUM_ARTIST, INNER_META_KEY_ALBUM_ARTIST),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_ARTIST, INNER_META_KEY_ARTIST),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_AUTHOR, INNER_META_KEY_AUTHOR),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_COMPOSER, INNER_META_KEY_COMPOSER),
    /**
     * The most of gst plugins don't send the GST_TAG_DURATION, we obtain this
     * information from duration query.
     */
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_DURATION, ""),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_GENRE, INNER_META_KEY_GENRE),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_HAS_AUDIO, ""),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_HAS_VIDEO, ""),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_MIME_TYPE, INNER_META_KEY_MIME_TYPE),
    /**
     * The GST_TAG_TRACK_COUNT does not means the actual track count, we obtain
     * this information from pads count.
     */
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_NUM_TRACKS, ""),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_SAMPLE_RATE, INNER_META_KEY_SAMPLE_RATE),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_TITLE, INNER_META_KEY_TITLE),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_VIDEO_HEIGHT, INNER_META_KEY_VIDEO_HEIGHT),
    AVMETA_KEY_TO_X_MAP_ITEM(AV_KEY_VIDEO_WIDTH, INNER_META_KEY_VIDEO_WIDTH),
};

void PopulateMeta(Metadata &meta)
{
    for (auto &item : AVMETA_KEY_TO_X_MAP) {
        if (!meta.HasMeta(item.first)) {
            meta.SetMeta(item.first, "");
        }
    }
}

struct AVMetaElemMetaCollector::TrackInfo {
    bool valid = true;
    Metadata uploadMeta;
    Format innerMeta;
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

AVMetaElemMetaCollector::AVMetaElemMetaCollector(AVMetaSourceType type, const MetaResCb &resCb)
    : type_(type), resCb_(resCb)
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR ", type: %{public}hhu", FAKE_POINTER(this), type_);
}

AVMetaElemMetaCollector::~AVMetaElemMetaCollector()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

void AVMetaElemMetaCollector::Stop()
{
    std::unique_lock<std::mutex> lock(mutex_);
    stopCollecting_ = true;

    for (auto &[elem, signalId] : signalIds_) {
        g_signal_handler_disconnect(elem, signalId);
    }
    signalIds_.clear();

    for (auto &[pad, probeId] : padProbes_) {
        gst_pad_remove_probe(pad, probeId);
    }
    padProbes_.clear();

    lock.unlock();
    lock.lock();
}

bool AVMetaElemMetaCollector::IsMetaCollected()
{
    std::unique_lock<std::mutex> lock(mutex_);
    for (auto &[dummy, trackInfo] : trackInfos_) {
        if (!trackInfo.valid) {
            continue;
        }
        if (trackInfo.innerMeta.GetFormatMap().empty()) {
            return false;
        }
    }

    // at least the duration meta and track count or container mime.
    if (fileUploadMeta_.tbl_.empty()) {
        return false;
    }

    return true;
}

std::shared_ptr<AVSharedMemory> AVMetaElemMetaCollector::FetchArtPicture()
{
    std::unique_lock<std::mutex> lock(mutex_);

    auto artPicMem = DoFetchArtPicture(fileInnerMeta_);
    if (artPicMem != nullptr) {
        return artPicMem;
    }

    for (auto &[dummy, trackInnerMeta] : trackInfos_) {
        artPicMem = DoFetchArtPicture(trackInnerMeta.innerMeta);
        if (artPicMem != nullptr) {
            return artPicMem;
        }
    }

    return artPicMem;
}

std::shared_ptr<AVSharedMemory> AVMetaElemMetaCollector::DoFetchArtPicture(const Format &innerMeta)
{
    if (!innerMeta.ContainKey(INNER_META_KEY_IMAGE)) {
        return nullptr;
    }

    MEDIA_LOGD("has art picture");

    uint8_t *addr = nullptr;
    size_t size = 0;
    (void)innerMeta.GetBuffer(INNER_META_KEY_IMAGE, &addr, size);

    static constexpr size_t maxImageSize = 1 * 1024 * 1024;
    if (addr == nullptr || size == 0 || size > maxImageSize) {
        MEDIA_LOGW("invalid param, size = %{public}zu", size);
        return nullptr;
    }

    auto artPicMem = AVSharedMemoryBase::CreateFromLocal(
        static_cast<int32_t>(size), AVSharedMemory::FLAGS_READ_ONLY, "artpic");
    CHECK_AND_RETURN_RET_LOG(artPicMem != nullptr, nullptr, "create art pic failed");

    errno_t rc = memcpy_s(artPicMem->GetBase(), static_cast<size_t>(artPicMem->GetSize()), addr, size);
    CHECK_AND_RETURN_RET_LOG(rc == EOK, nullptr, "memcpy_s failed");

    return artPicMem;
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
    std::unique_lock<std::mutex> lock(mutex_);
    if (stopCollecting_) {
        MEDIA_LOGI("stop collecting...");
        return false;
    }

    gulong probeId = gst_pad_add_probe(&pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, ProbeCallback, this, nullptr);
    if (probeId == 0) {
        MEDIA_LOGE("add probe for %{public}s's pad %{public}s failed", PAD_PARENT_NAME(&pad), PAD_NAME(&pad));
        return false;
    }

    (void)padProbes_.emplace(&pad, probeId);
    (void)trackInfos_.emplace(&pad, TrackInfo {});

    // report the track count change when caps arrived.
    trackcount_ += 1;
    MEDIA_LOGD("add probe to pad %{public}s of %{public}s", PAD_NAME(&pad), PAD_PARENT_NAME(&pad));
    return true;
}

bool AVMetaElemMetaCollector::ConnectSignal(GstElement &elem, std::string_view signal, GCallback callback)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (stopCollecting_) {
        MEDIA_LOGI("stop collecting...");
        return false;
    }

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
    if (!trackInfo.valid) {
        return;
    }

    GstTagScope scope = gst_tag_list_get_scope(&tagList);
    MEDIA_LOGI("catch tag %{public}s event", scope == GST_TAG_SCOPE_GLOBAL ? "global" : "stream");

    if (scope == GST_TAG_SCOPE_GLOBAL) {
        if (globalTagCatched_) {
            return;
        }
        globalTagCatched_ = true;
    }

    if (scope == GST_TAG_SCOPE_GLOBAL) {
        GstMetaParser::ParseTagList(tagList, fileInnerMeta_);
        ConvertToAVMeta(fileInnerMeta_, fileUploadMeta_);
        ReportMeta(fileUploadMeta_);
    } else {
        GstMetaParser::ParseTagList(tagList, trackInfo.innerMeta);
        ConvertToAVMeta(trackInfo.innerMeta, trackInfo.uploadMeta);
        ReportMeta(trackInfo.uploadMeta);
    }
}

void AVMetaElemMetaCollector::ParseCaps(const GstCaps &caps, TrackInfo &trackInfo)
{
    GstMetaParser::ParseStreamCaps(caps, trackInfo.innerMeta);

    if (!EnsureTrackValid(trackInfo)) {
        return;
    }

    fileUploadMeta_.SetMeta(AV_KEY_NUM_TRACKS, std::to_string(trackcount_));
    ReportMeta(fileUploadMeta_);

    ConvertToAVMeta(trackInfo.innerMeta, trackInfo.uploadMeta);
    ReportMeta(trackInfo.uploadMeta);
}

bool AVMetaElemMetaCollector::EnsureTrackValid(TrackInfo &trackInfo)
{
    /**
     * If the track can not supported, it would not be taken account into the
     * total track counts. The ffmpeg will generate one track for one image of
     * the metadata, the track's caps is image/png or image/jpeg, etc. For such
     * tracks, them should be considered as invalid tracks.
     */
    int32_t trackType;
    std::string mimeType;
    if (!trackInfo.innerMeta.GetIntValue(INNER_META_KEY_TRACK_TYPE, trackType) ||
        !trackInfo.innerMeta.GetStringValue(INNER_META_KEY_MIME_TYPE, mimeType)) {
        trackInfo.valid = false;
        trackcount_ -= 1;
        fileUploadMeta_.SetMeta(AV_KEY_NUM_TRACKS, std::to_string(trackcount_));
        ReportMeta(fileUploadMeta_);
        return false;
    }

    if (trackType == MediaType::MEDIA_TYPE_VID) {
        trackInfo.uploadMeta.SetMeta(AV_KEY_HAS_VIDEO, "yes");
    } else if (trackType == MediaType::MEDIA_TYPE_AUD) {
        trackInfo.uploadMeta.SetMeta(AV_KEY_HAS_AUDIO, "yes");
    }

    return true;
}

void AVMetaElemMetaCollector::OnEventProbe(GstPad &pad, GstEvent &event)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (stopCollecting_) {
        MEDIA_LOGI("stop collecting...");
        return;
    }

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

        static constexpr int32_t NASEC_PER_HALF_MILLISEC = 500000;
        static constexpr int32_t NASEC_PER_MILLISEC = 1000000;

        int64_t milliSecond;
        if ((std::numeric_limits<int64_t>::max() - NASEC_PER_HALF_MILLISEC) < duration_) {
            milliSecond = duration_ / NASEC_PER_MILLISEC; // ns -> ms
        } else {
            milliSecond = (duration_ + NASEC_PER_HALF_MILLISEC) / NASEC_PER_MILLISEC; // ns -> ms, round up.
        }

        fileUploadMeta_.SetMeta(AV_KEY_DURATION, std::to_string(milliSecond));
        ReportMeta(fileUploadMeta_);
    }
}

void AVMetaElemMetaCollector::ReportMeta(const Metadata &uploadMeta)
{
    if (resCb_ == nullptr) {
        return;
    }

    mutex_.unlock();
    resCb_(uploadMeta);
    mutex_.lock();
}

void AVMetaElemMetaCollector::ConvertToAVMeta(const Format &innerMeta, Metadata &avmeta) const
{
    for (const auto &[avKey, innerKey] : AVMETA_KEY_TO_X_MAP) {
        if (innerKey.compare("") == 0) {
            continue;
        }

        if (innerKey.compare(INNER_META_KEY_MIME_TYPE) == 0) { // only need the file mime type
            continue;
        }

        if (!innerMeta.ContainKey(innerKey)) {
            continue;
        }

        std::string strVal;
        int32_t intVal;
        FormatDataType type = innerMeta.GetValueType(innerKey);
        switch (type) {
            case FORMAT_TYPE_STRING:
                innerMeta.GetStringValue(innerKey, strVal);
                avmeta.SetMeta(avKey, strVal);
                break;
            case FORMAT_TYPE_INT32:
                innerMeta.GetIntValue(innerKey, intVal);
                avmeta.SetMeta(avKey, std::to_string(intVal));
                break;
            default:
                break;
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
    std::unique_lock<std::mutex> lock(mutex_);
    if (stopCollecting_) {
        MEDIA_LOGI("stop collecting...");
        return;
    }

    GstMetaParser::ParseFileMimeType(caps, fileInnerMeta_);

    std::string mimeType;
    (void)fileInnerMeta_.GetStringValue(INNER_META_KEY_MIME_TYPE, mimeType);
    fileUploadMeta_.SetMeta(AV_KEY_MIME_TYPE, mimeType);

    ReportMeta(fileUploadMeta_);
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
} // namespace Media
} // namespace OHOS