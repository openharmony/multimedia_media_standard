/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License\n");
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

#include "avcodeclist_demo.h"
#include <iostream>
#include "media_errors.h"

using namespace std;

namespace OHOS {
namespace Media {
void AVCodecListDemo::DoNext()
{
    cout << "Enter your step:" << endl;
    std::string cmd;
    Format format; // The content of format should be optional
    std::string codecName;
    while (std::getline(std::cin, cmd)) {
        if (cmd.find("fvd") != std::string::npos || cmd.find("FindVideoDecoder") != std::string::npos) {
            codecName = avCodecList_->FindVideoDecoder(format);
            cout << "FindVideoDecoder : " << codecName << endl;
        } else if (cmd.find("fve") != std::string::npos || cmd.find("FindVideoEncoder") != std::string::npos) {
            codecName = avCodecList_->FindVideoEncoder(format);
            cout << "FindVideoEncoder : " << codecName << endl;
        } else if (cmd.find("fad") != std::string::npos || cmd.find("FindAudioDecoder") != std::string::npos) {
            codecName = avCodecList_->FindAudioDecoder(format);
            cout << "FindAudioDecoder : " << codecName << endl;
        } else if (cmd.find("fae") != std::string::npos || cmd.find("FindAudioEncoder") != std::string::npos) {
            codecName = avCodecList_->FindAudioEncoder(format);
            cout << "FindAudioEncoder : " << codecName << endl;
        } else if (cmd.find("gvd") != std::string::npos || cmd.find("GetVideoDecoderCaps") != std::string::npos) {
            std::vector<std::shared_ptr<VideoCaps>> videoDecoderArray = avCodecList_->GetVideoDecoderCaps();
            PrintVideoCapsArray(videoDecoderArray);
        } else if (cmd.find("gve") != std::string::npos || cmd.find("GetVideoEncoderCaps") != std::string::npos) {
            std::vector<std::shared_ptr<VideoCaps>> videoEncoderArray = avCodecList_->GetVideoEncoderCaps();
            PrintVideoCapsArray(videoEncoderArray);
        } else if (cmd.find("gad") != std::string::npos || cmd.find("GetAudioDecoderCaps") != std::string::npos) {
            std::vector<std::shared_ptr<AudioCaps>> audioDecoderArray = avCodecList_->GetAudioDecoderCaps();
            PrintAudioCapsArray(audioDecoderArray);
        } else if (cmd.find("gae") != std::string::npos || cmd.find("GetAudioEncoderCaps") != std::string::npos) {
            std::vector<std::shared_ptr<AudioCaps>> audioEncoderArray = avCodecList_->GetAudioEncoderCaps();
            PrintAudioCapsArray(audioEncoderArray);
        }
    }
}

void AVCodecListDemo::PrintVideoCapsArray(const std::vector<std::shared_ptr<VideoCaps>> &videoCapsArray) const
{
    for (auto iter = videoCapsArray.begin(); iter != videoCapsArray.end(); iter++) {
        std::shared_ptr<VideoCaps> pVideoCaps = *iter;
        std::shared_ptr<AVCodecInfo> pVideoCodecCaps;
        pVideoCodecCaps = pVideoCaps->GetCodecInfo();
        cout << "This codec capability is :" << endl;
        cout << "GetName = "<< pVideoCodecCaps->GetName() << endl;
        cout << "GetType = "<< pVideoCodecCaps->GetType() << endl;
        cout << "GetMimeType = "<< pVideoCodecCaps->GetMimeType() << endl;
        cout << "IsHardwareAccelerated = "<< pVideoCodecCaps->IsHardwareAccelerated() << endl;
        cout << "IsSoftwareOnly = "<< pVideoCodecCaps->IsSoftwareOnly() << endl;
        cout << "IsVendor = "<< pVideoCodecCaps->IsVendor() << endl;
        cout << "GetSupportedBitrate = "<< pVideoCaps->GetSupportedBitrate().minVal <<\
                " - " << pVideoCaps->GetSupportedBitrate().maxVal << endl;
        cout << "GetSupportedWidthAlignment = "<< pVideoCaps->GetSupportedWidthAlignment() << endl;
        cout << "GetSupportedHeightAlignment = "<< pVideoCaps->GetSupportedHeightAlignment() << endl;
        cout << "GetSupportedWidth = "<< pVideoCaps->GetSupportedWidth().minVal <<\
                " - " << pVideoCaps->GetSupportedWidth().maxVal << endl;
        cout << "GetSupportedHeight = "<< pVideoCaps->GetSupportedHeight().minVal <<\
                " - " << pVideoCaps->GetSupportedHeight().maxVal << endl;
        cout << "GetSupportedFrameRate = "<< pVideoCaps->GetSupportedFrameRate().minVal <<\
                " - " << pVideoCaps->GetSupportedFrameRate().maxVal << endl;
        cout << "GetSupportedEncodeQuality = "<< pVideoCaps->GetSupportedEncodeQuality().minVal <<\
                " - " << pVideoCaps->GetSupportedEncodeQuality().maxVal << endl;
        cout << "GetSupportedQuality = "<< pVideoCaps->GetSupportedQuality().minVal <<\
                " - " << pVideoCaps->GetSupportedQuality().maxVal << endl;
        PrintIntArray(pVideoCaps->GetSupportedFormats(), "GetSupportedFormats");
        PrintIntArray(pVideoCaps->GetSupportedProfiles(), "GetSupportedProfiles");
        PrintIntArray(pVideoCaps->GetSupportedBitrateMode(), "GetSupportedBitrateMode");
        PrintIntArray(pVideoCaps->GetSupportedLevels(), "GetSupportedBitrateMode");
    }
}

void AVCodecListDemo::PrintAudioCapsArray(const std::vector<std::shared_ptr<AudioCaps>> &audioCapsArray) const
{
    for (auto iter = audioCapsArray.begin(); iter != audioCapsArray.end(); iter++) {
        std::shared_ptr<AudioCaps> pAudioCaps = *iter;
        std::shared_ptr<AVCodecInfo> pAudioCodecCaps = pAudioCaps->GetCodecInfo();
        cout << "This codec capability is :" << endl;
        cout << "GetName = "<< pAudioCodecCaps->GetName() << endl;
        cout << "GetType = "<< pAudioCodecCaps->GetType() << endl;
        cout << "GetMimeType = "<< pAudioCodecCaps->GetMimeType() << endl;
        cout << "IsHardwareAccelerated = "<< pAudioCodecCaps->IsHardwareAccelerated() << endl;
        cout << "IsSoftwareOnly = "<< pAudioCodecCaps->IsSoftwareOnly() << endl;
        cout << "IsVendor = "<< pAudioCodecCaps->IsVendor() << endl;
        cout << "GetSupportedBitrate = "<< pAudioCaps->GetSupportedBitrate().minVal <<\
                " - " << pAudioCaps->GetSupportedBitrate().maxVal << endl;
        cout << "GetSupportedChannel = "<< pAudioCaps->GetSupportedChannel().minVal <<\
                " - " << pAudioCaps->GetSupportedChannel().maxVal << endl;
        cout << "GetSupportedComplexity = "<< pAudioCaps->GetSupportedComplexity().minVal <<\
                " - "<< pAudioCaps->GetSupportedComplexity().maxVal << endl;
        PrintIntArray(pAudioCaps->GetSupportedFormats(), "GetSupportedFormats");
        PrintIntArray(pAudioCaps->GetSupportedSampleRates(), "GetSupportedSampleRates");
        PrintIntArray(pAudioCaps->GetSupportedProfiles(), "GetSupportedProfiles");
        PrintIntArray(pAudioCaps->GetSupportedLevels(), "GetSupportedBitrateMode");
    }
}

void AVCodecListDemo::PrintIntArray(const std::vector<int32_t> &array, const std::string &logmsg) const
{
    cout << logmsg << ": ";
    for (auto iter = array.begin(); iter != array.end(); iter++) {
        cout << *iter << ", ";
    }
    cout  << endl;
}

void AVCodecListDemo::RunCase(const string &path)
{
    avCodecList_ = OHOS::Media::AVCodecListFactory::CreateAVCodecList();
    if (avCodecList_ == nullptr) {
        cout << "avCodecList_ is null" << endl;
        return;
    }
    
    DoNext();
}
}
}