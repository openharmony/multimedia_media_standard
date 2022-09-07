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

#include "avcodeclist_demo.h"
#include <iostream>
#include "media_errors.h"
#include "string_ex.h"

using namespace std;

namespace OHOS {
namespace Media {
void AVCodecListDemo::DoNext()
{
    cout << "Enter your step:" << endl;
    std::string cmd;
    Format format;
    std::string codecName;
    while (std::getline(std::cin, cmd)) {
        if (cmd.find("fvd") != std::string::npos || cmd.find("FindVideoDecoder") != std::string::npos) {
            if (BuildFormat(format)) {
                codecName = avCodecList_->FindVideoDecoder(format);
                cout << "FindVideoDecoder : " << codecName << endl;
            }
        } else if (cmd.find("fve") != std::string::npos || cmd.find("FindVideoEncoder") != std::string::npos) {
            if (BuildFormat(format)) {
                codecName = avCodecList_->FindVideoEncoder(format);
                cout << "FindVideoEncoder : " << codecName << endl;
            }
        } else if (cmd.find("fad") != std::string::npos || cmd.find("FindAudioDecoder") != std::string::npos) {
            if (BuildFormat(format)) {
                codecName = avCodecList_->FindAudioDecoder(format);
                cout << "FindAudioDecoder : " << codecName << endl;
            }
        } else if (cmd.find("fae") != std::string::npos || cmd.find("FindAudioEncoder") != std::string::npos) {
            if (BuildFormat(format)) {
                codecName = avCodecList_->FindAudioEncoder(format);
                cout << "FindAudioEncoder : " << codecName << endl;
            }
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
        } else if (cmd.find("gsfr") != std::string::npos || cmd.find("GetSupportedFrameRates") != std::string::npos) {
            GetSupportedFrameRatesDemo();
        } else if (cmd.find("gpfr") != std::string::npos || cmd.find("GetPreferredFrameRate") != std::string::npos) {
            GetPreferredFrameRateDemo();
        }
    }
}

void AVCodecListDemo::GetSupportedFrameRatesDemo()
{
    Range ret;
    ImgSize size = SetSize();

    std::vector<std::shared_ptr<VideoCaps>> videoEncoderArray = avCodecList_->GetVideoEncoderCaps();
    for (auto iter = videoEncoderArray.begin(); iter != videoEncoderArray.end(); iter++) {
        ret = (*iter)->GetSupportedFrameRatesFor(size.width, size.height);
        cout << "name = " << (*iter)->GetCodecInfo()->GetName() << ":" << endl;
        cout << "framerate = " << ret.minVal << ", " << ret.maxVal << endl;
    }
    std::vector<std::shared_ptr<VideoCaps>> videoDecoderArray = avCodecList_->GetVideoDecoderCaps();
    for (auto iter = videoDecoderArray.begin(); iter != videoDecoderArray.end(); iter++) {
        ret = (*iter)->GetSupportedFrameRatesFor(size.width, size.height);
        cout << "name = " << (*iter)->GetCodecInfo()->GetName() << ":" << endl;
        cout << "framerate = " << ret.minVal << ", " << ret.maxVal << endl;
    }
}

void AVCodecListDemo::GetPreferredFrameRateDemo()
{
    Range ret;
    ImgSize size = SetSize();

    std::vector<std::shared_ptr<VideoCaps>> videoEncoderArray = avCodecList_->GetVideoEncoderCaps();
    for (auto iter = videoEncoderArray.begin(); iter != videoEncoderArray.end(); iter++) {
        ret = (*iter)->GetPreferredFrameRate(size.width, size.height);
        cout << "name = " << (*iter)->GetCodecInfo()->GetName() << ":" << endl;
        cout << "framerate = " << ret.minVal << ", " << ret.maxVal << endl;
    }
    std::vector<std::shared_ptr<VideoCaps>> videoDecoderArray = avCodecList_->GetVideoDecoderCaps();
    for (auto iter = videoDecoderArray.begin(); iter != videoDecoderArray.end(); iter++) {
        ret = (*iter)->GetPreferredFrameRate(size.width, size.height);
        cout << "name = " << (*iter)->GetCodecInfo()->GetName() << ":" << endl;
        cout << "framerate = " << ret.minVal << ", " << ret.maxVal << endl;
    }
}

ImgSize AVCodecListDemo::SetSize()
{
    string str;
    cout << "Enter width : " << endl;
    (void)getline(cin, str);
    int32_t width = -1;
    if (!StrToInt(str, width)) {
        cout << "call StrToInt func false, input str is:" << str.c_str();
        return ImgSize(0, 0);
    }
    cout << "Enter height : " << endl;

    (void)getline(cin, str);
    int32_t height = -1;
    if (!StrToInt(str, height)) {
        cout << "call StrToInt func false, input str is:" << str.c_str();
        return ImgSize(0, 0);
    }
    cout << "width = " << width << ", height = " << height << endl;
    return ImgSize(width, height);
}

bool AVCodecListDemo::BuildFormat(Format &format)
{
    format = Format();
    const std::unordered_map<std::string, int32_t> MIME_TO_MEDIA_TYPE_MAP = {
        {"audio/mpeg", MEDIA_TYPE_AUD},
        {"audio/mp4a-latm", MEDIA_TYPE_AUD},
        {"audio/vorbis", MEDIA_TYPE_AUD},
        {"audio/flac", MEDIA_TYPE_AUD},
        {"video/avc", MEDIA_TYPE_VID},
        {"video/h263", MEDIA_TYPE_VID},
        {"video/mpeg2", MEDIA_TYPE_VID},
        {"video/mp4v-es", MEDIA_TYPE_VID},
        {"video/x-vnd.on2.vp8", MEDIA_TYPE_VID}
    };

    cout << "Enter the MediaDescription of codec :" << endl;
    cout << "Enter the codec mime :" << endl;
    std::string codecMime;
    (void)getline(cin, codecMime);
    if (codecMime == "") {
        cout << "Failed! The target of codec_mime cannot be set to null " << endl;
        return false;
    }
    format.PutStringValue("codec_mime", codecMime);
    (void)SetMediaDescriptionToFormat(format, "bitrate");

    if (MIME_TO_MEDIA_TYPE_MAP.find(codecMime) != MIME_TO_MEDIA_TYPE_MAP.end()) {
        if (MIME_TO_MEDIA_TYPE_MAP.at(codecMime) == MEDIA_TYPE_VID) {
            (void)SetMediaDescriptionToFormat(format, "width");
            (void)SetMediaDescriptionToFormat(format, "height");
            (void)SetMediaDescriptionToFormat(format, "pixel_format");
            (void)SetMediaDescriptionToFormat(format, "frame_rate");
        } else if (MIME_TO_MEDIA_TYPE_MAP.at(codecMime) == MEDIA_TYPE_AUD) {
            (void)SetMediaDescriptionToFormat(format, "channel_count");
            (void)SetMediaDescriptionToFormat(format, "samplerate");
        }
    } else {
        cout << "Failed! The target of codec_mime cannot be set to " << codecMime << endl;
        return false;
    }

    return true;
}

void AVCodecListDemo::SetMediaDescriptionToFormat(Format &format, const std::string &key)
{
    cout << "Set the " << key << " :" << endl;
    string mediaDescription;
    if (key == "pixel_format") {
        cout << "1:YUVI420" << endl;
        cout << "2:NV12" << endl;
        cout << "3:NV21" << endl;
        cout << "4:RGBA" << endl;
    }
    (void)getline(cin, mediaDescription);
    if (mediaDescription == "") {
        cout << key << " is setting to null" << endl;
        return;
    }
    int32_t number = -1;
    if (!StrToInt(mediaDescription, number)) {
        cout << "call StrToInt func false, input str is:" << mediaDescription.c_str();
        return;
    }
    format.PutIntValue(key, number);
    cin.clear();
}

void AVCodecListDemo::PrintVideoCapsArray(const std::vector<std::shared_ptr<VideoCaps>> &videoCapsArray) const
{
    for (auto iter = videoCapsArray.begin(); iter != videoCapsArray.end(); iter++) {
        std::shared_ptr<VideoCaps> pVideoCaps = *iter;
        if (pVideoCaps == nullptr) {
            cout << "pVideoCaps is nullptr" << endl;
            break;
        }
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
        if (pAudioCodecCaps == nullptr) {
            cout << "pAudioCodecCaps is nullptr" << endl;
            break;
        }
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
} // namespace Media
} // namespace OHOS