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

#include "avmeta_meta_collector.h"
#include "avmetadatahelper.h"
#include "media_errors.h"
#include "media_log.h"
#include "gst_meta_parser.h"
#include "gst_utils.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMetaCollector"};
}

namespace OHOS {
namespace Media {
enum GstElemType : uint8_t {
    TYPEFIND,
    DEMUXER,
    PARSER,
    DECODER,
    DECODEBIN,
    UNKNOWN,
};

struct GstElemMetaMatchDesc {
    std::string_view metaKey;
    std::vector<std::string_view> expectedFields;
};

static const std::unordered_map<GstElemType, GstElemMetaMatchDesc> GST_ELEM_META_MATCH_DESC = {
    { GstElemType::TYPEFIND, { GST_ELEMENT_METADATA_LONGNAME, { "TypeFind" } } },
    { GstElemType::DEMUXER, { GST_ELEMENT_METADATA_KLASS, { "Codec", "Demuxer" } } },
    { GstElemType::PARSER, { GST_ELEMENT_METADATA_KLASS, { "Codec", "Parser" } } },
    { GstElemType::DECODER, { GST_ELEMENT_METADATA_KLASS, { "Codec", "Decoder" } } },
    { GstElemType::DECODEBIN, { GST_ELEMENT_METADATA_LONGNAME, { "Decoder Bin" } } },
};

/**
 * @brief limit the multiqueue's cache limit to avoid the waste of memory.
 * For metadata and thubnail scene, there is no need to cache too much
 * buffer in the queue.
 */
class AVMetaMetaCollector::MultiQueueCutOut {
public:
    explicit MultiQueueCutOut(GstElement &mq) : mq_(GST_ELEMENT_CAST(gst_object_ref(&mq)))
    {
        g_object_get(mq_, "max-size-bytes", &maxBytes_, "max-size-buffers",
            &maxBuffers_, "max-size-time", &maxTimes_, nullptr);
        MEDIA_LOGI("mq curr maxBytes: %{public}u, maxBuffers: %{public}u, maxTimes: %{public}" PRIu64,
            maxBytes_, maxBytes_, maxTimes_);

        static constexpr uint32_t maxBytes = 2 * 1024 * 1024;
        static constexpr uint32_t maxBuffers = 5;
        static constexpr uint64_t maxTimes = 2 * GST_SECOND;
        g_object_set(mq_, "max-size-bytes", maxBytes, "max-size-buffers",
            maxBuffers, "max-size-time", maxTimes, nullptr);
    }

    ~MultiQueueCutOut()
    {
        if (isHiden_) {
            gst_object_unref(mq_);
            return;
        }

        g_object_set(mq_, "max-size-bytes", maxBytes_, "max-size-buffers",
            maxBuffers_, "max-size-time", maxTimes_, nullptr);
        gst_object_unref(mq_);
    }

    void Hide()
    {
        isHiden_ = true;
    }

private:
    GstElement *mq_;
    uint32_t maxBuffers_ = 0;
    uint32_t maxBytes_ = 0;
    uint64_t maxTimes_ = 0;
    bool isHiden_ = false;
};

AVMetaMetaCollector::AVMetaMetaCollector()
{
    MEDIA_LOGD("enter ctor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
}

AVMetaMetaCollector::~AVMetaMetaCollector()
{
    MEDIA_LOGD("enter dtor, instance: 0x%{public}06" PRIXPTR "", FAKE_POINTER(this));
    std::unique_lock<std::mutex> lock(mutex_);
    {
        decltype(elemCollectors_) temp;
        temp.swap(elemCollectors_);
    }
    {
        decltype(blockers_) temp;
        temp.swap(blockers_);
    }
}

void AVMetaMetaCollector::Start()
{
    MEDIA_LOGD("start collecting...");

    std::unique_lock<std::mutex> lock(mutex_);
    if (stopCollecting_ || !allMeta_.tbl_.empty()) {
        return;
    }
}

void AVMetaMetaCollector::AddMetaSource(GstElement &source)
{
    MEDIA_LOGD("enter");

    std::unique_lock<std::mutex> lock(mutex_);
    if (stopCollecting_) {
        return;
    }

    uint8_t srcType = ProbeElemType(source);
    AddElemCollector(source, srcType);
    AddElemBlocker(source, srcType);
}

void AVMetaMetaCollector::Stop(bool unlock) /* false */
{
    MEDIA_LOGD("stop collecting...");

    std::unique_lock<std::mutex> lock(mutex_);
    stopCollecting_ = true;
    cond_.notify_all();

    StopBlocker(unlock);

    for (auto &elemCollector : elemCollectors_) {
        elemCollector->Stop();
    }
}

std::unordered_map<int32_t, std::string> AVMetaMetaCollector::GetMetadata()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]() { return CheckCollectCompleted() || stopCollecting_; });

    AdjustMimeType();
    PopulateMeta(allMeta_);

    return allMeta_.tbl_;
}

std::string AVMetaMetaCollector::GetMetadata(int32_t key)
{
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this, key]() {
        return stopCollecting_ || allMeta_.HasMeta(key) || CheckCollectCompleted();
    });

    AdjustMimeType();

    std::string result;
    (void)allMeta_.TryGetMeta(key, result);
    return result;
}

std::shared_ptr<AVSharedMemory> AVMetaMetaCollector::FetchArtPicture()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]() { return CheckCollectCompleted() || stopCollecting_; });

    std::shared_ptr<AVSharedMemory> result = nullptr;
    for (auto &elemCollector : elemCollectors_) {
        if (elemCollector != nullptr) {
            result = elemCollector->FetchArtPicture();
        }
        if (result != nullptr) {
            break;
        }
    }

    return result;
}

bool AVMetaMetaCollector::CheckCollectCompleted() const
{
    if (elemCollectors_.size() == 0 || blockers_.size() == 0) {
        return false;
    }

    for (auto &collector : elemCollectors_) {
        if (collector == nullptr) {
            continue;
        }

        if (!collector->IsMetaCollected()) {
            return false;
        }
    }

    for (auto &[type, blockerVec] : blockers_) {
        for (auto &blocker : blockerVec) {
            if (blocker == nullptr) {
                continue;
            }
            if (!blocker->IsRemoved() && !blocker->IsBufferDetected()) {
                return false;
            }
        }
    }

    MEDIA_LOGI("collect metadata finished !");
    return true;
}

uint8_t AVMetaMetaCollector::ProbeElemType(GstElement &source)
{
    for (const auto &[srcType, matchDesc] : GST_ELEM_META_MATCH_DESC) {
        bool matchResult = MatchElementByMeta(source, matchDesc.metaKey, matchDesc.expectedFields);
        if (!matchResult) {
            continue;
        }

        std::string detailLog = "metaKey: ";
        detailLog += matchDesc.metaKey;
        detailLog += ", expected field: ";
        for (auto &fields : matchDesc.expectedFields) {
            detailLog += fields;
            detailLog += " ";
        }
        MEDIA_LOGD("find %{public}s, %{public}s", ELEM_NAME(&source), detailLog.c_str());

        if (hasSrcType_.count(srcType) == 0) {
            (void)hasSrcType_.emplace(srcType, 0);
        }
        hasSrcType_[srcType] += 1;
        return srcType;
    }

    return GstElemType::UNKNOWN;
}

void AVMetaMetaCollector::AdjustMimeType()
{
    std::string mimeType = allMeta_.GetMeta(AV_KEY_MIME_TYPE);
    if (mimeType.empty()) {
        return;
    }

    if (mimeType.compare(FILE_MIMETYPE_VIDEO_MP4) == 0) {
        std::string hasVideo = allMeta_.GetMeta(AV_KEY_HAS_VIDEO);
        if (hasVideo.compare("yes") == 0) {
            return;
        }
        std::string hasAudio = allMeta_.GetMeta(AV_KEY_HAS_AUDIO);
        if (hasAudio.compare("yes") == 0) {
            allMeta_.SetMeta(AV_KEY_MIME_TYPE, std::string(FILE_MIMETYPE_AUDIO_MP4));
            return;
        }
    }
}

void AVMetaMetaCollector::UpdataMeta(const Metadata &metadata)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (stopCollecting_) {
        return;
    }

    for (auto &[key, value] : metadata.tbl_) {
        allMeta_.SetMeta(key, value);
    }

    cond_.notify_all();
}

void AVMetaMetaCollector::AddElemCollector(GstElement &source, uint8_t type)
{
    if (type == GstElemType::DECODEBIN) {
        mqCutOut_ = std::make_unique<MultiQueueCutOut>(source);
        return;
    }

    if (type != GstElemType::TYPEFIND && type != GstElemType::DEMUXER && type != GstElemType::PARSER) {
        return;
    }

    for (auto &collector : elemCollectors_) {
        if (collector->GetType() == static_cast<AVMetaSourceType>(type)) {
            collector->AddMetaSource(source);
            return;
        }
    }

    // already has demuxer, reject to create parser's collector
    if ((hasSrcType_.count(GstElemType::DEMUXER) != 0) &&
        (type != GstElemType::DEMUXER) &&
        (type != GstElemType::TYPEFIND)) {
        return;
    }

    auto metaUpdateCb = std::bind(&AVMetaMetaCollector::UpdataMeta, this, std::placeholders::_1);
    auto result = AVMetaElemMetaCollector::Create(static_cast<AVMetaSourceType>(type), metaUpdateCb);
    CHECK_AND_RETURN(result != nullptr);
    result->AddMetaSource(source);
    elemCollectors_.push_back(std::move(result));
}

void AVMetaMetaCollector::AddElemBlocker(GstElement &source, uint8_t type)
{
    /**
     * After the demuxer or parser plugin of gstreamer complete the metadata resolve work,
     * them will send one frame buffer to downstream. If there is decoder at the downstream,
     * the decode will happened, which is unnecessary and wastefully for metadata resolving.
     * We can block the downstream pads of demuxer to prevent the decode process happened.
     *
     * One kind of possible sequence of element setuped to the pipeline is :
     * Demuxer1 --> Demuxer2 ---> Parser1 --> Decoder1
     *                      |\
     *                      | `-> Parser2 --> Parser3 --> Decoder2
     *                      \
     *                       `--> Decoder3
     * Or:
     * Parser1 --> Decoder1
     * Or:
     * Parser1 -->
     *
     * Therefore, we will process the block by referring to these order.
     */

#define PUSH_NEW_BLOCK(type, blocker)                                         \
    do {                                                                      \
        auto typeBlockersIter = blockers_.find(type);                         \
        if (typeBlockersIter == blockers_.end()) {                            \
            auto ret = blockers_.emplace(type, BufferBlockerVec {});          \
            typeBlockersIter = ret.first;                                     \
        }                                                                     \
        (blocker)->Init();                                                    \
        (void)typeBlockersIter->second.emplace_back(blocker);      \
    } while (0)

    if (type == GstElemType::TYPEFIND || type == GstElemType::DECODEBIN || type == GstElemType::UNKNOWN) {
        return;
    }

    auto notifier = [this]() {
        // get lock to ensure the notification will take effect.
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.notify_all();
    };

    if (type == GstElemType::DEMUXER) {
        auto blocker = std::make_shared<AVMetaBufferBlocker>(source, true, notifier);
        PUSH_NEW_BLOCK(type, blocker);
        return;
    }

    if (type == GstElemType::DECODER) {
        auto blocker = std::make_shared<AVMetaBufferBlocker>(source, false, notifier);
        PUSH_NEW_BLOCK(type, blocker);
        return;
    }

    if (type == GstElemType::PARSER) {
        /**
         * If there is a demuxer, we can not add blocker at the parser's srcpad, the parser
         * maybe need to wait several packets of buffer to autoplug the decoder, which will
         * leads to no buffer can arrived at the srcpad of parser due to the MultiQueueCutOut.
         * Insteadly, we add the blocker at the parser's sinkpad to fix this issue.
         *
         */
        if (hasSrcType_.count(GstElemType::DEMUXER) != 0) {
            auto blocker = std::make_shared<AVMetaBufferBlocker>(source, false, notifier);
            PUSH_NEW_BLOCK(type, blocker);
        } else {
            auto blocker = std::make_shared<AVMetaBufferBlocker>(source, true, notifier);
            PUSH_NEW_BLOCK(type, blocker);
        }
        return;
    }
}

void AVMetaMetaCollector::StopBlocker(bool unlock)
{
    for (auto &[type, blockerVec] : blockers_) {
        for (auto &blocker : blockerVec) {
            if (blocker == nullptr) {
                continue;
            }
            // place the if-else at the for-loop for cyclomatic complexity
            if (unlock) {
                blocker->Remove();
            } else {
                blocker->Hide();
            }
        }
    }

    if (mqCutOut_ != nullptr && !unlock) {
        mqCutOut_->Hide();
    }
    mqCutOut_ = nullptr; // restore the mq's cache limit
}
} // namespace Media
} // namespace OHOS
