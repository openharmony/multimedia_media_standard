/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include "avcodec_info_native_mock.h"
#include "avformat_native_mock.h"
#include "avcodec_list_native_mock.h"

namespace OHOS {
namespace Media {
std::string AVCodecListNativeMock::FindVideoDecoder(std::shared_ptr<FormatMock> format) const
{
    std::string ret;
    if (avCodecList_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        ret = avCodecList_->FindVideoDecoder(fmt->GetFormat());
    }
    return ret;
}

std::string AVCodecListNativeMock::FindVideoEncoder(std::shared_ptr<FormatMock> format) const
{
    std::string ret;
    if (avCodecList_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        ret = avCodecList_->FindVideoEncoder(fmt->GetFormat());
    }
    return ret;
}

std::string AVCodecListNativeMock::FindAudioDecoder(std::shared_ptr<FormatMock> format) const
{
    std::string ret;
    if (avCodecList_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        ret = avCodecList_->FindAudioDecoder(fmt->GetFormat());
    }
    return ret;
}

std::string AVCodecListNativeMock::FindAudioEncoder(std::shared_ptr<FormatMock> format) const
{
    std::string ret;
    if (avCodecList_ != nullptr && format != nullptr) {
        auto fmt = std::static_pointer_cast<AVFormatNativeMock>(format);
        ret = avCodecList_->FindAudioEncoder(fmt->GetFormat());
    }
    return ret;
}


std::vector<std::shared_ptr<VideoCapsMock>> AVCodecListNativeMock::GetVideoDecoderCaps() const
{
    std::vector<std::shared_ptr<VideoCaps>> videoCapsArray;
    std::vector<std::shared_ptr<VideoCapsMock>> retVideoCapsArray;
    if (avCodecList_ == nullptr) {
        return retVideoCapsArray;
    }
    videoCapsArray = avCodecList_->GetVideoDecoderCaps();
    for (auto iter = videoCapsArray.begin(); iter != videoCapsArray.end(); iter++) {
        std::shared_ptr<VideoCaps> pVideoCaps = *iter;
        if (pVideoCaps == nullptr) {
            break;
        }
        retVideoCapsArray.push_back(std::make_shared<VideoCapsNativeMock>(pVideoCaps));
    }
    return retVideoCapsArray;
}

std::vector<std::shared_ptr<VideoCapsMock>> AVCodecListNativeMock::GetVideoEncoderCaps() const
{
    std::vector<std::shared_ptr<VideoCaps>> videoCapsArray;
    std::vector<std::shared_ptr<VideoCapsMock>> retVideoCapsArray;
    if (avCodecList_ == nullptr) {
        return retVideoCapsArray;
    }
    videoCapsArray = avCodecList_->GetVideoEncoderCaps();
    for (auto iter = videoCapsArray.begin(); iter != videoCapsArray.end(); iter++) {
        std::shared_ptr<VideoCaps> pVideoCaps = *iter;
        if (pVideoCaps == nullptr) {
            break;
        }
        retVideoCapsArray.push_back(std::make_shared<VideoCapsNativeMock>(pVideoCaps));
    }
    return retVideoCapsArray;
}

std::vector<std::shared_ptr<AudioCapsMock>> AVCodecListNativeMock::GetAudioDecoderCaps() const
{
    std::vector<std::shared_ptr<AudioCaps>> audioCapsArray;
    std::vector<std::shared_ptr<AudioCapsMock>> retAudioCapsArray;
    if (avCodecList_ == nullptr) {
        return retAudioCapsArray;
    }
    audioCapsArray = avCodecList_->GetAudioDecoderCaps();
    for (auto iter = audioCapsArray.begin(); iter != audioCapsArray.end(); iter++) {
        std::shared_ptr<AudioCaps> pAudioCaps = *iter;
        if (pAudioCaps == nullptr) {
            break;
        }
        retAudioCapsArray.push_back(std::make_shared<AudioCapsNativeMock>(pAudioCaps));
    }
    return retAudioCapsArray;
}

std::vector<std::shared_ptr<AudioCapsMock>> AVCodecListNativeMock::GetAudioEncoderCaps() const
{
    std::vector<std::shared_ptr<AudioCaps>> audioCapsArray;
    std::vector<std::shared_ptr<AudioCapsMock>> retAudioCapsArray;
    if (avCodecList_ == nullptr) {
        return retAudioCapsArray;
    }
    audioCapsArray = avCodecList_->GetAudioEncoderCaps();
    for (auto iter = audioCapsArray.begin(); iter != audioCapsArray.end(); iter++) {
        std::shared_ptr<AudioCaps> pAudioCaps = *iter;
        if (pAudioCaps == nullptr) {
            break;
        }
        retAudioCapsArray.push_back(std::make_shared<AudioCapsNativeMock>(pAudioCaps));
    }
    return retAudioCapsArray;
}
} // Media
} // OHOS