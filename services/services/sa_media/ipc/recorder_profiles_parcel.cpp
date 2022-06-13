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

#include "recorder_profiles_parcel.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "RecorderProfilesParcel"};
}

namespace OHOS {
namespace Media {
void RecorderProfilesParcel::MarshallingRecorderProfilesData(
    MessageParcel &parcel, const RecorderProfilesData &profileCapabilityData)
{
    //  string
    (void)parcel.WriteString(profileCapabilityData.videoCaps.containerFormatType);
    (void)parcel.WriteString(profileCapabilityData.audioCaps.containerFormatType);
    (void)parcel.WriteString(profileCapabilityData.recorderProfile.containerFormatType);
    (void)parcel.WriteString(profileCapabilityData.audioCaps.mimeType);
    (void)parcel.WriteString(profileCapabilityData.videoCaps.audioEncoderMime);
    (void)parcel.WriteString(profileCapabilityData.videoCaps.videoEncoderMime);
    (void)parcel.WriteString(profileCapabilityData.recorderProfile.audioCodec);
    (void)parcel.WriteString(profileCapabilityData.recorderProfile.videoCodec);

    //  Range
    (void)parcel.WriteInt32(profileCapabilityData.audioCaps.bitrate.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.audioCaps.bitrate.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.audioCaps.channels.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.audioCaps.channels.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.audioBitrateRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.audioBitrateRange.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.audioChannelRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.audioChannelRange.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.videoBitrateRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.videoBitrateRange.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.videoFramerateRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.videoFramerateRange.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.videoWidthRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.videoWidthRange.maxVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.videoHeightRange.minVal);
    (void)parcel.WriteInt32(profileCapabilityData.videoCaps.videoHeightRange.maxVal);

    //  int32_t
    (void)parcel.WriteInt32(profileCapabilityData.sourceId);
    (void)parcel.WriteInt32(profileCapabilityData.mediaProfileType);
    (void)parcel.WriteInt32(profileCapabilityData.recorderProfile.audioBitrate);
    (void)parcel.WriteInt32(profileCapabilityData.recorderProfile.audioChannels);
    (void)parcel.WriteInt32(profileCapabilityData.recorderProfile.audioSampleRate);
    (void)parcel.WriteInt32(profileCapabilityData.recorderProfile.durationTime);
    (void)parcel.WriteInt32(profileCapabilityData.recorderProfile.qualityLevel);
    (void)parcel.WriteInt32(profileCapabilityData.recorderProfile.videoBitrate);
    (void)parcel.WriteInt32(profileCapabilityData.recorderProfile.videoFrameWidth);
    (void)parcel.WriteInt32(profileCapabilityData.recorderProfile.videoFrameHeight);
    (void)parcel.WriteInt32(profileCapabilityData.recorderProfile.videoFrameRate);

    //  std::vector<int32_t>
    (void)parcel.WriteInt32Vector(profileCapabilityData.audioCaps.sampleRate);
    (void)parcel.WriteInt32Vector(profileCapabilityData.videoCaps.audioSampleRates);
}

bool RecorderProfilesParcel::Marshalling(
    MessageParcel &parcel, const std::vector<RecorderProfilesData> &profileCapabilityDataArray)
{
    parcel.WriteUint32(profileCapabilityDataArray.size());
    for (auto it = profileCapabilityDataArray.begin(); it != profileCapabilityDataArray.end(); it++) {
        MarshallingRecorderProfilesData(parcel, (*it));
    }
    MEDIA_LOGD("success to Marshalling profileCapabilityDataArray");

    return true;
}

bool RecorderProfilesParcel::Marshalling(MessageParcel &parcel, const RecorderProfilesData &profileCapabilityData)
{
    MarshallingRecorderProfilesData(parcel, profileCapabilityData);
    MEDIA_LOGD("success to Marshalling profileCapabilityData");

    return true;
}

void RecorderProfilesParcel::UnmarshallingRecorderProfilesData(
    MessageParcel &parcel, RecorderProfilesData &profileCapabilityData)
{
    //  string
    profileCapabilityData.videoCaps.containerFormatType = parcel.ReadString();
    profileCapabilityData.audioCaps.containerFormatType = parcel.ReadString();
    profileCapabilityData.recorderProfile.containerFormatType = parcel.ReadString();
    profileCapabilityData.audioCaps.mimeType = parcel.ReadString();
    profileCapabilityData.videoCaps.audioEncoderMime = parcel.ReadString();
    profileCapabilityData.videoCaps.videoEncoderMime = parcel.ReadString();
    profileCapabilityData.recorderProfile.audioCodec = parcel.ReadString();
    profileCapabilityData.recorderProfile.videoCodec = parcel.ReadString();

    //  Range
    profileCapabilityData.audioCaps.bitrate.minVal = parcel.ReadInt32();
    profileCapabilityData.audioCaps.bitrate.maxVal = parcel.ReadInt32();
    profileCapabilityData.audioCaps.channels.minVal = parcel.ReadInt32();
    profileCapabilityData.audioCaps.channels.maxVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.audioBitrateRange.minVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.audioBitrateRange.maxVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.audioChannelRange.minVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.audioChannelRange.maxVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.videoBitrateRange.minVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.videoBitrateRange.maxVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.videoFramerateRange.minVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.videoFramerateRange.maxVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.videoWidthRange.minVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.videoWidthRange.maxVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.videoHeightRange.minVal = parcel.ReadInt32();
    profileCapabilityData.videoCaps.videoHeightRange.maxVal = parcel.ReadInt32();

    //  int32_t
    profileCapabilityData.sourceId = parcel.ReadInt32();
    profileCapabilityData.mediaProfileType = parcel.ReadInt32();
    profileCapabilityData.recorderProfile.audioBitrate = parcel.ReadInt32();
    profileCapabilityData.recorderProfile.audioChannels = parcel.ReadInt32();
    profileCapabilityData.recorderProfile.audioSampleRate = parcel.ReadInt32();
    profileCapabilityData.recorderProfile.durationTime = parcel.ReadInt32();
    profileCapabilityData.recorderProfile.qualityLevel = parcel.ReadInt32();
    profileCapabilityData.recorderProfile.videoBitrate = parcel.ReadInt32();
    profileCapabilityData.recorderProfile.videoFrameWidth = parcel.ReadInt32();
    profileCapabilityData.recorderProfile.videoFrameHeight = parcel.ReadInt32();
    profileCapabilityData.recorderProfile.videoFrameRate = parcel.ReadInt32();

    //  std::vector<int32_t>
    parcel.ReadInt32Vector(&profileCapabilityData.audioCaps.sampleRate);
    parcel.ReadInt32Vector(&profileCapabilityData.videoCaps.audioSampleRates);
}

bool RecorderProfilesParcel::Unmarshalling(
    MessageParcel &parcel, std::vector<RecorderProfilesData> &profileCapabilityDataArray)
{
    uint32_t size = parcel.ReadUint32();
    for (uint32_t index = 0; index < size; index++) {
        RecorderProfilesData profileCapabilityData;
        UnmarshallingRecorderProfilesData(parcel, profileCapabilityData);
        profileCapabilityDataArray.push_back(profileCapabilityData);
    }
    MEDIA_LOGD("success to Unmarshalling profileCapabilityDataArray");

    return true;
}

bool RecorderProfilesParcel::Unmarshalling(MessageParcel &parcel, RecorderProfilesData &profileCapabilityData)
{
    UnmarshallingRecorderProfilesData(parcel, profileCapabilityData);

    MEDIA_LOGD("success to Unmarshalling profileCapabilityData");
    return true;
}
}  // namespace Media
}  // namespace OHOS
