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

#ifndef AVMETA_ELEM_META_COLLECTOR_H
#define AVMETA_ELEM_META_COLLECTOR_H

#include <functional>
#include <unordered_map>
#include <string>
#include <string_view>
#include <vector>
#include <mutex>
#include <memory>
#include <nocopyable.h>
#include <gst/gst.h>
#include "gst_meta_parser.h"

namespace OHOS {
namespace Media {
// indicate this number is not track's number, just represent the whole file.
inline constexpr int32_t AVMETA_TRACK_NUMBER_FILE = -1;
/* param: trackId, metadata */
using MetaResCb = std::function<void(int32_t, const Metadata &)>;

enum class AVMetaSourceType : uint8_t {
    TYPEFIND,
    DEMUXER,
    PARSER,
};

class AVMetaElemMetaCollector {
public:
    AVMetaElemMetaCollector(AVMetaSourceType type, const MetaResCb &resCb);
    virtual ~AVMetaElemMetaCollector();

    static std::unique_ptr<AVMetaElemMetaCollector> Create(AVMetaSourceType type, const MetaResCb &resCb);
    static Metadata GetDefaultMeta();

    virtual void AddMetaSource(GstElement &elem) = 0;
    int32_t GetTrackCount();
    AVMetaSourceType GetType() const
    {
        return type_;
    }
    DISALLOW_COPY_AND_MOVE(AVMetaElemMetaCollector);

protected:
    bool AddProbeToPadList(GList &list);
    bool AddProbeToPad(GstPad &pad);
    bool ConnectSignal(GstElement &elem, std::string_view signal, GCallback callback);
    void ReportMeta(int32_t trackId, const Metadata &metadata);

private:
    static GstPadProbeReturn ProbeCallback(GstPad *pad, GstPadProbeInfo *info, gpointer usrdata);
    void OnEventProbe(GstPad &pad, GstEvent &event);
    void QueryDuration(GstPad &pad);

    struct TrackInfo;
    void ParseTagList(const GstTagList &tagList, TrackInfo &trackInfo);
    void ParseCaps(const GstCaps &caps, TrackInfo &trackInfo);
    void ConvertToAVMeta(const Metadata &innerMeta, Metadata &avmeta) const;

    AVMetaSourceType type_;
    MetaResCb resCb_;
    std::unordered_map<GstPad *, gulong> padProbes_;
    std::unordered_map<GstElement *, gulong> signalIds_;
    std::unordered_map<GstPad *, TrackInfo> trackInfos_;
    Metadata fileMeta_;
    bool fileMetaUpdated_ = false;
    std::mutex trackInfoMutex_;
    int64_t duration_ = 0;
    bool globalTagCatched_ = false;
};

/**
 * Detail Element Meta Collector Implementation Begin.
 */
class TypeFindMetaCollector : public AVMetaElemMetaCollector {
public:
    using AVMetaElemMetaCollector::AVMetaElemMetaCollector;
    ~TypeFindMetaCollector() = default;

    void AddMetaSource(GstElement &elem) override;

private:
    static void HaveTypeCallback(GstElement *elem, guint probability, GstCaps *caps, gpointer userdata);
    void OnHaveType(const GstElement &elem, const GstCaps &caps);
};

class DemuxerMetaCollector : public AVMetaElemMetaCollector {
public:
    using AVMetaElemMetaCollector::AVMetaElemMetaCollector;
    ~DemuxerMetaCollector() = default;

    void AddMetaSource(GstElement &elem) override;

private:
    static void PadAddedCallback(GstElement *elem, GstPad *pad, gpointer userdata);
    void OnPadAdded(GstElement &src, GstPad &pad);
};

class ParserMetaCollector : public AVMetaElemMetaCollector {
public:
    using AVMetaElemMetaCollector::AVMetaElemMetaCollector;
    ~ParserMetaCollector() = default;

    void AddMetaSource(GstElement &elem) override;
};
}
}

#endif
