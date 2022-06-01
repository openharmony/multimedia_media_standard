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

#include "recorder_profiles_demo.h"
#include <iostream>
#include "media_errors.h"
#include "string_ex.h"
#include "avmuxer_demo.h"

using namespace std;

namespace OHOS {
namespace Media {
void RecorderProfilesDemo::PrintInputInfo() const
{
    cout << "Please select a test api: " << endl;
    cout << "1:IsAudioRecoderConfigSupported" << endl;
    cout << "2:HasVideoRecorderProfile" << endl;
    cout << "3:GetAudioRecorderCaps" << endl;
    cout << "4:GetVideoRecorderCaps" << endl;
    cout << "5:GetVideoRecorderProfile" << endl;
    cout << "6:GetAVMuxerFormatList" << endl;
    cout << endl;
}

void RecorderProfilesDemo::DoNext()
{
    cout << endl;
    cout << "Enter your step:" << endl;
    std::string cmd;
    bool result;

    PrintInputInfo();
    while (std::getline(std::cin, cmd)) {
        if (cmd.find("1") != std::string::npos || cmd.find("IsAudioRecoderConfigSupported") != std::string::npos) {
            if (CreatProfile()) {
                result = OHOS::Media::RecorderProfilesFactory::CreateRecorderProfiles().IsAudioRecoderConfigSupported(
                    *profile_);
                cout << "IsAudioRecoderConfigSupported : " << result << endl;
                profile_ = nullptr;
            }
        } else if (cmd.find("2") != std::string::npos || cmd.find("HasVideoRecorderProfile") != std::string::npos) {
            if (BuildSourceId()) {
                result = OHOS::Media::RecorderProfilesFactory::CreateRecorderProfiles().HasVideoRecorderProfile(
                    sourceId_, qualityLevel_);
                cout << "HasVideoRecorderProfile : " << result<< endl;
            }
        } else if (cmd.find("3") != std::string::npos || cmd.find("AudioRecorderCaps") != std::string::npos) {
            std::vector<std::shared_ptr<AudioRecorderCaps>> audioRecorderArray =
                OHOS::Media::RecorderProfilesFactory::CreateRecorderProfiles().GetAudioRecorderCaps();
            PrintAudioRecorderCapsArray(audioRecorderArray);
        } else if (cmd.find("4") != std::string::npos || cmd.find("GetVideoRecorderCaps") != std::string::npos) {
            std::vector<std::shared_ptr<VideoRecorderCaps>> videoRecorderArray =
                OHOS::Media::RecorderProfilesFactory::CreateRecorderProfiles().GetVideoRecorderCaps();
            PrintVideoRecorderCapsArray(videoRecorderArray);
        } else if (cmd.find("5") != std::string::npos || cmd.find("GetVideoRecorderProfile") != std::string::npos) {
            if (BuildSourceId()) {
                std::shared_ptr<VideoRecorderProfile> videoRecorderProfile =
                    OHOS::Media::RecorderProfilesFactory::CreateRecorderProfiles().GetVideoRecorderProfile(
                        sourceId_, qualityLevel_);
                PrintVideoRecorderProfile(videoRecorderProfile);
            }
        } else if (cmd.find("6") != std::string::npos || cmd.find("GetAVMuxerFormatList") != std::string::npos) {
            std::vector<std::string> muxerFormatList =
                OHOS::Media::AVMuxerFactory::CreateAVMuxer()->GetAVMuxerFormatList();
            PrintMuxerFormatList(muxerFormatList);
        }
        cout << endl;
        PrintInputInfo();
    }
}

bool RecorderProfilesDemo::CreatProfile()
{
    cout << "please input the AudioRecoderConfig."<< endl;
    profile_  = std::make_shared<AudioRecorderProfile>();
    bool ret = true;
    ret = (ret && SetStringParamValue("containerFormatType"));
    ret = (ret && SetStringParamValue("audioCodec"));
    ret = (ret && SetIntParamValue("audioBitrate"));
    ret = (ret && SetIntParamValue("audioSampleRate"));
    ret = (ret && SetIntParamValue("audioChannels"));
    return ret;
}

bool RecorderProfilesDemo::BuildSourceId()
{
    bool ret = true;
    ret = (ret && SetIntParamValue("sourceId"));
    ret = (ret && SetIntParamValue("qualityLevel"));
    return ret;
}

bool RecorderProfilesDemo::SetIntParamValue(const std::string &strKey)
{
    cout << "please input the " << strKey <<" :"<< endl;
    std::string inputValue = "";
    (void)getline(cin, inputValue);
    if (inputValue == "") {
        cout << "input value is null" << endl;
        return false;
    }

    int32_t number = -1;
    if (!StrToInt(inputValue, number)) {
        cout << "call StrToInt func false, input str is:" << inputValue.c_str();
        return false;
    }

    if ("sourceId" == strKey) {
        sourceId_ = number;
    } else if ("qualityLevel" == strKey) {
        qualityLevel_ = number;
    } else if ("audioBitrate" == strKey) {
        profile_->audioBitrate = number;
    } else if ("audioSampleRate" == strKey) {
        profile_->audioSampleRate = number;
    } else if ("audioChannels" == strKey) {
        profile_->audioChannels = number;
    } else {
        cout << "input wrong key" << endl;
    }

    cin.clear();
    return true;
}

bool RecorderProfilesDemo::SetStringParamValue(const std::string &strKey)
{
    cout << "please input the " << strKey <<" :"<< endl;
    std::string inputValue = "";
    (void)getline(cin, inputValue);
    if (inputValue == "") {
        cout << "input value is null" << endl;
        return false;
    }

    if ("containerFormatType" == strKey) {
        profile_->containerFormatType = inputValue;
    } else if ("audioCodec" == strKey) {
        profile_->audioCodec = inputValue;
    } else {
        cout << "input wrong key" << endl;
    }

    cin.clear();
    return true;
}

void RecorderProfilesDemo::PrintAudioRecorderCapsArray(
    const std::vector<std::shared_ptr<AudioRecorderCaps>> &audioRecorderArray) const
{
    for (auto iter = audioRecorderArray.begin(); iter != audioRecorderArray.end(); iter++) {
        std::shared_ptr<AudioRecorderCaps> pAudioRecorderCaps = *iter;
        cout << "GetContainerFormatType = " <<  pAudioRecorderCaps->containerFormatType << endl;
        cout << "GetMimeType = "<<  pAudioRecorderCaps->mimeType << endl;
        cout << "GetSupportedBitrate = "<< pAudioRecorderCaps->bitrate.minVal << \
            " - " << pAudioRecorderCaps->bitrate.maxVal << endl;
        cout << "GetSupportedChannel = "<< pAudioRecorderCaps->channels.minVal << \
            " - " << pAudioRecorderCaps->channels.maxVal << endl;
        PrintIntArray(pAudioRecorderCaps->sampleRate, "GetSupportedSampleRates");
    }
}

void RecorderProfilesDemo::PrintVideoRecorderCapsArray(
    const std::vector<std::shared_ptr<VideoRecorderCaps>> &videoRecorderArray) const
{
    for (auto iter =  videoRecorderArray.begin(); iter !=  videoRecorderArray.end(); iter++) {
        std::shared_ptr< VideoRecorderCaps> pVideoRecorderCaps = *iter;
        cout << "GetContainerFormatType = " <<  pVideoRecorderCaps->containerFormatType << endl;
        cout << "GetAudioEncoderMime = " << pVideoRecorderCaps->audioEncoderMime << endl;
        cout << "GetVideoEncoderMime = " <<  pVideoRecorderCaps->videoEncoderMime <<endl;
        cout << "GetSupportedAudioBitrate = "<< pVideoRecorderCaps->audioBitrateRange.minVal << \
            " - " << pVideoRecorderCaps->audioBitrateRange.maxVal << endl;
        cout << "GetSupportedChannel = "<< pVideoRecorderCaps->audioChannelRange.minVal << \
            " - " << pVideoRecorderCaps->audioChannelRange.maxVal << endl;
        cout << "GetSupportedVideoBitrate = "<< pVideoRecorderCaps->videoBitrateRange.minVal << \
            " - "<< pVideoRecorderCaps->videoBitrateRange.maxVal << endl;
        cout << "GetSupportedVideoFramerate = "<< pVideoRecorderCaps->videoFramerateRange.minVal << \
            " - "<< pVideoRecorderCaps->videoFramerateRange.maxVal << endl;
        cout << "GetSupportedVideoWidthRange = "<< pVideoRecorderCaps->videoWidthRange.minVal << \
            " - "<< pVideoRecorderCaps->videoWidthRange.maxVal << endl;
        cout << "GetSupportedVideoHeightRange = "<< pVideoRecorderCaps->videoHeightRange.minVal << \
            " - "<< pVideoRecorderCaps->videoHeightRange.maxVal << endl;
        PrintIntArray(pVideoRecorderCaps->audioSampleRates, "GetSupportedSampleRates");
    }
}

void RecorderProfilesDemo::PrintMuxerFormatList(
    const std::vector<std::string> &formatList) const
{
    int count = 0;
    cout << "MuxerFormat" << ": " << endl;
    for (auto iter = formatList.begin(); iter != formatList.end(); iter++) {
        count++;
        cout << count <<":" << *iter << ", " << endl;
    }
    cout  << endl;
}

void RecorderProfilesDemo::PrintVideoRecorderProfile(
    const std::shared_ptr<VideoRecorderProfile> &videoRecorderProfile) const
{
        cout << "GetContainerFormatType = " <<  videoRecorderProfile->containerFormatType << endl;
        cout << "GetAudioBitrate = "<< videoRecorderProfile->audioBitrate << endl;
        cout << "GetAudioChannels = " << videoRecorderProfile->audioChannels<< endl;
        cout << "GetAudioCodec = " <<  videoRecorderProfile->audioCodec <<endl;
        cout << "GetAudioSampleRate = "<< videoRecorderProfile->audioSampleRate << endl;
        cout << "GetDurationTime = "<< videoRecorderProfile->durationTime << endl;
        cout << "GetQualityLevel = "<< videoRecorderProfile->qualityLevel << endl;
        cout << "GetVideoBitrate = "<< videoRecorderProfile->videoBitrate << endl;
        cout << "GetVideoCodec = "<< videoRecorderProfile->videoCodec << endl;
        cout << "GetVideoFrameWidth = "<< videoRecorderProfile->videoFrameWidth << endl;
        cout << "GetVideoFrameHeight = "<< videoRecorderProfile->videoFrameHeight << endl;
        cout << "GetVideoFrameRate = "<< videoRecorderProfile->videoFrameRate << endl;
}

void RecorderProfilesDemo::PrintIntArray(const std::vector<int32_t> &array, const std::string &logmsg) const
{
    cout << logmsg << ": ";
    for (auto iter = array.begin(); iter != array.end(); iter++) {
        cout << *iter << ", ";
    }
    cout  << endl;
}

void RecorderProfilesDemo::RunCase(const string &path)
{
    DoNext();
}
}  // namespace Media
}  // namespace OHOS