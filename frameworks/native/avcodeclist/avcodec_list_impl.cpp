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

#include "avcodec_list_impl.h"
#include "media_log.h"
#include "media_errors.h"
#include "i_media_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecListImpl"};
}
namespace OHOS {
namespace Media {
std::shared_ptr<AVCodecList> AVCodecListFactory::CreateAVCodecList()
{
    std::shared_ptr<AVCodecListImpl> impl = std::make_shared<AVCodecListImpl>();
    CHECK_AND_RETURN_RET_LOG(impl != nullptr, nullptr, "failed to new AVCodecListImpl");

    int32_t ret = impl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, nullptr, "failed to init AVCodecListImpl");

    return impl;
}

int32_t AVCodecListImpl::Init()
{
    codecListService_ = MediaServiceFactory::GetInstance().CreateAVCodecListService();
    CHECK_AND_RETURN_RET_LOG(codecListService_ != nullptr, MSERR_UNKNOWN, "failed to create AVCodecList service");
    return MSERR_OK;
}

AVCodecListImpl::AVCodecListImpl()
{
    MEDIA_LOGD("AVCodecListImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecListImpl::~AVCodecListImpl()
{
    if (codecListService_ != nullptr) {
        (void)MediaServiceFactory::GetInstance().DestroyAVCodecListService(codecListService_);
        codecListService_ = nullptr;
    }
    MEDIA_LOGD("AVCodecListImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::string AVCodecListImpl::FindVideoDecoder(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecListService_ != nullptr, "", "AvCodecList service does not exist..");
    return codecListService_->FindVideoDecoder(format);
}

std::string AVCodecListImpl::FindVideoEncoder(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecListService_ != nullptr, "", "AvCodecList service does not exist..");
    return codecListService_->FindVideoEncoder(format);
}

std::string AVCodecListImpl::FindAudioDecoder(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecListService_ != nullptr, "", "AvCodecList service does not exist..");
    return codecListService_->FindAudioDecoder(format);
}

std::string AVCodecListImpl::FindAudioEncoder(const Format &format)
{
    CHECK_AND_RETURN_RET_LOG(codecListService_ != nullptr, "", "AvCodecList service does not exist..");
    return codecListService_->FindAudioEncoder(format);
}

std::vector<CapabilityData> AVCodecListImpl::GetCodecCapabilityInfos()
{
    if (!capabilityArray_.empty()) {
        MEDIA_LOGD("capabilityArray_ has been assigned.");
        return capabilityArray_;
    }
    return codecListService_->GetCodecCapabilityInfos();
}

std::vector<std::shared_ptr<VideoCaps>> AVCodecListImpl::GetVideoDecoderCaps()
{
    if (!videoDecoderCapsArray_.empty()) {
        MEDIA_LOGD("videoDecoderCapsArray_ has been assigned.");
        return videoDecoderCapsArray_;
    }
    if (capabilityArray_.empty()) {
        capabilityArray_ = GetCodecCapabilityInfos();
    }
    std::vector<CapabilityData> vdecCapabilityArray = SelectTargetCapabilityDataArray(AVCODEC_TYPE_VIDEO_DECODER);
    for (auto iter = vdecCapabilityArray.begin(); iter != vdecCapabilityArray.end(); iter++) {
        std::shared_ptr<VideoCaps> videoCaps = std::make_shared<VideoCaps>(*iter);
        CHECK_AND_RETURN_RET_LOG(videoCaps != nullptr, videoDecoderCapsArray_, "Is null mem");
        videoDecoderCapsArray_.push_back(videoCaps);
    }
    return videoDecoderCapsArray_;
}

std::vector<std::shared_ptr<VideoCaps>> AVCodecListImpl::GetVideoEncoderCaps()
{
    if (!videoEncoderCapsArray_.empty()) {
        MEDIA_LOGD("videoEncoderCapsArray_ has been assigned.");
        return videoEncoderCapsArray_;
    }
    if (capabilityArray_.empty()) {
        capabilityArray_ = GetCodecCapabilityInfos();
    }
    std::vector<CapabilityData> vencCapabilityArray = SelectTargetCapabilityDataArray(AVCODEC_TYPE_VIDEO_ENCODER);
    for (auto iter = vencCapabilityArray.begin(); iter != vencCapabilityArray.end(); iter++) {
        std::shared_ptr<VideoCaps> videoCaps = std::make_shared<VideoCaps>(*iter);
        CHECK_AND_RETURN_RET_LOG(videoCaps != nullptr, videoEncoderCapsArray_, "Is null mem");
        videoEncoderCapsArray_.push_back(videoCaps);
    }
    return videoEncoderCapsArray_;
}

std::vector<std::shared_ptr<AudioCaps>> AVCodecListImpl::GetAudioDecoderCaps()
{
    if (!audioDecoderCapsArray_.empty()) {
        MEDIA_LOGD("audioDecoderCapsArray_ has been assigned.");
        return audioDecoderCapsArray_;
    }
    if (capabilityArray_.empty()) {
        capabilityArray_ = GetCodecCapabilityInfos();
    }
    std::vector<CapabilityData> adecCapabilityArray = SelectTargetCapabilityDataArray(AVCODEC_TYPE_AUDIO_DECODER);
    for (auto iter = adecCapabilityArray.begin(); iter != adecCapabilityArray.end(); iter++) {
        std::shared_ptr<AudioCaps> audioCaps = std::make_shared<AudioCaps>(*iter);
        CHECK_AND_RETURN_RET_LOG(audioCaps != nullptr, audioDecoderCapsArray_, "Is null mem");
        audioDecoderCapsArray_.push_back(audioCaps);
    }
    return audioDecoderCapsArray_;
}

std::vector<std::shared_ptr<AudioCaps>> AVCodecListImpl::GetAudioEncoderCaps()
{
    if (!audioEncoderCapsArray_.empty()) {
        MEDIA_LOGD("audioEncoderCapsArray_ has been assigned.");
        return audioEncoderCapsArray_;
    }
    if (capabilityArray_.empty()) {
        capabilityArray_ = GetCodecCapabilityInfos();
    }
    std::vector<CapabilityData> aencCapabilityArray = SelectTargetCapabilityDataArray(AVCODEC_TYPE_AUDIO_ENCODER);
    for (auto iter = aencCapabilityArray.begin(); iter != aencCapabilityArray.end(); iter++) {
        std::shared_ptr<AudioCaps> audioCaps = std::make_shared<AudioCaps>(*iter);
        CHECK_AND_RETURN_RET_LOG(audioCaps != nullptr, audioEncoderCapsArray_, "Is null mem");
        audioEncoderCapsArray_.push_back(audioCaps);
    }
    return audioEncoderCapsArray_;
}

std::vector<CapabilityData> AVCodecListImpl::SelectTargetCapabilityDataArray(const AVCodecType &codecType) const
{
    std::vector<CapabilityData> targetCapabilityData;
    for (auto iter = capabilityArray_.begin(); iter != capabilityArray_.end(); iter++) {
        if (iter->codecType == codecType) {
            targetCapabilityData.push_back(*iter);
        }
    }
    return targetCapabilityData;
}
} // namespace Media
} // namespace OHOS