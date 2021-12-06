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
};

AVMetaMetaCollector::AVMetaMetaCollector()
    : currSetupedElemType_(GstElemType::UNKNOWN)
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

    {
        decltype(signalIds_) temp;
        temp.swap(signalIds_);

        for (auto &[elem, signalId] : temp) {
            g_signal_handler_disconnect(elem, signalId);
        }
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

bool AVMetaMetaCollector::CheckCollectCompleted() const
{
    if (elemCollectors_.size() == 0 || blockers_.size() == 0) {
        return false;
    }

    for (auto &collector : elemCollectors_) {
        if (collector == nullptr) {
            continue;
        }

        if (collector->GetType() == AVMetaSourceType::TYPEFIND) {
            if (trackMetaCollected_.count(AVMETA_TRACK_NUMBER_FILE) == 0) {
                return false;
            }
            continue;
        }

        int32_t trackCount = collector->GetTrackCount();
        if (trackCount == 0) {
            return false;
        }

        for (auto trackId = 0; trackId < trackCount; trackId++) {
            if (trackMetaCollected_.count(trackId) == 0) {
                return false;
            }
        }
    }

    for (auto &[type, blockerVec] : blockers_) {
        for (auto &blocker : blockerVec) {
            if (blocker == nullptr) {
                continue;
            }
            if (blocker->GetStreamCount() == 0 || !blocker->CheckBufferRecieved()) {
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

void AVMetaMetaCollector::UpdataMeta(int32_t trackId, const Metadata &metadata)
{
    MEDIA_LOGD("trackId = %{public}d", trackId);
    std::unique_lock<std::mutex> lock(mutex_);
    if (stopCollecting_) {
        return;
    }

    for (auto &[key, value] : metadata.tbl_) {
        allMeta_.SetMeta(key, value);
    }

    (void)trackMetaCollected_.emplace(trackId);
    cond_.notify_all();
}

void AVMetaMetaCollector::AddElemCollector(GstElement &source, uint8_t type)
{
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

    auto metaUpdateCb = std::bind(&AVMetaMetaCollector::UpdataMeta,
                                  this, std::placeholders::_1, std::placeholders::_2);
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
     * the decode will happened, which is unneccesary and wastefully for metadata resolving.
     * We can block the demuxer or parser's sinkpads to prevent the decode process happened.
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

    if (type == GstElemType::TYPEFIND || type == GstElemType::UNKNOWN) {
        return;
    }

    /**
     * We only move the blocker to downstream if the lastest blocker comes from the demuxer, for
     * avoiding delete the blocker incorrectly, because we can not figure out the direct upstream
     * element if the lastest blocker does not come from the demuxer. The playbin's auto-pluggging
     * mechanism always guarantees the fact that when the demuxer's new pad added, the first downstream
     * element for this new pad will be found and connected immediately. Then, based on this fact, we
     * can properly move the lastest blocker to the current setuped element when the lastest blocker
     * comes from the demuxer.
     *
     * Considering this element setuped order: demuxer, audio parser, video decoder, audio decoder.
     */
    if (currSetupedElemType_ != GstElemType::UNKNOWN && currSetupedElemType_ != GstElemType::DEMUXER) {
        return;
    }

    auto notifier = [this]() {
        // get lock to ensure the notification will take effect.
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.notify_all();
    };

    if (type == GstElemType::DEMUXER || type == GstElemType::PARSER) {
        auto blocker = std::make_shared<AVMetaBufferBlocker>(source, true, notifier);
        PUSH_NEW_BLOCK(type, blocker);
        UpdateElemBlocker(source, type);
        return;
    }

    if (type == GstElemType::DECODER) {
        auto blocker = std::make_shared<AVMetaBufferBlocker>(source, false, notifier);
        PUSH_NEW_BLOCK(type, blocker);
        UpdateElemBlocker(source, type);
    }
}

void AVMetaMetaCollector::UpdateElemBlocker(GstElement &source, uint8_t elemType)
{
    /**
     * When the new element is setuped, we need to update the block. The update
     * strategy is: cancel the upstream block. Because a block has been added to
     * the current element, after an upstream block is canceled, the block is
     * successfully moved downstream.
     *
     * When the demuxer is setuped, we always need to set the pad-added listener to
     * the demuxer so that we can figure out which element is upstream of the element
     * currently installed.
     */
    if (elemType == GstElemType::DEMUXER) {
        auto signalId = g_signal_connect(&source, "pad-added", G_CALLBACK(PadAdded), this);
        if (signalId == 0) {
            MEDIA_LOGE("add pad-added signal to %{public}s failed", ELEM_NAME(&source));
            return;
        }
        (void)signalIds_.emplace_back(std::pair<GstElement *, gulong>{&source, signalId});
    }

    MEDIA_LOGD("update blocker when elem %{public}s setup, elemType: %{public}hhu",
               ELEM_NAME(&source), elemType);

    do {
        /* We don't need to cancel the lastest blocker if the current setuped element is the first one. */
        if (currSetupedElemType_ == GstElemType::UNKNOWN) {
            break;
        }
        auto typeBlockersIter = blockers_.find(currSetupedElemType_);
        if (typeBlockersIter == blockers_.end() || typeBlockersIter->second.size() <= currSetupedElemIdx_) {
            break;
        }
        auto &currBlocker = typeBlockersIter->second[currSetupedElemIdx_];
        if (currBlocker == nullptr) {
            break;
        }
        if (currBlocker->GetStreamCount() == 0) {
            break;
        }
        currBlocker->CancelBlock(currBlocker->GetStreamCount() - 1);
        if (currSetupedElemType_ != GstElemType::DEMUXER) {
            currBlocker = nullptr;
        }
    } while (0);

    currSetupedElemType_ = elemType;
    // when this function invoked, the blocker had already been setuped for curr element.
    currSetupedElemIdx_ = blockers_.at(elemType).size() - 1;
    MEDIA_LOGD("currType = %{public}hhu, currIdx = %{public}zu", currSetupedElemType_, currSetupedElemIdx_);

    /**
     * Is there any such situation: there is no parser or decoder at the downstream of
     * a certain outstream of the demuxer.
     */
}

void AVMetaMetaCollector::PadAdded(GstElement *elem, GstPad *pad, gpointer userdata)
{
    (void)elem;
    (void)pad;
    CHECK_AND_RETURN_LOG(userdata != nullptr, "userdata is nullptr");
    auto collector = reinterpret_cast<AVMetaMetaCollector *>(userdata);
    std::unique_lock<std::mutex> lock(collector->mutex_);
    collector->currSetupedElemType_ = GstElemType::DEMUXER;
    // when padadded notify, the demuxer had already been setup blocker
    collector->currSetupedElemIdx_ = collector->blockers_.at(GstElemType::DEMUXER).size() - 1;
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
                blocker->CancelBlock(-1);
            } else {
                blocker->Clear();
            }
        }
    }
}
}
}
