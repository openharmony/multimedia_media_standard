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

#include "avcodeclist_parcel.h"
#include "media_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecListParcel"};
}

namespace OHOS {
namespace Media {
bool AVCodecListParcel::Marshalling(MessageParcel &parcel, const std::vector<CapabilityData> &capabilityDataArray)
{
    parcel.WriteUint32(capabilityDataArray.size());
    for (auto it = capabilityDataArray.begin(); it != capabilityDataArray.end(); it++) {
        (void)parcel.WriteString(it->codecName);
        (void)parcel.WriteString(it->mimeType);
        (void)parcel.WriteBool(it->isVendor);
        (void)parcel.WriteInt32(it->codecType);
        (void)parcel.WriteInt32(it->bitrate.minVal);
        (void)parcel.WriteInt32(it->bitrate.maxVal);
        (void)parcel.WriteInt32(it->channels.minVal);
        (void)parcel.WriteInt32(it->channels.maxVal);
        (void)parcel.WriteInt32(it->complexity.minVal);
        (void)parcel.WriteInt32(it->complexity.maxVal);
        (void)parcel.WriteInt32(it->alignment.minVal);
        (void)parcel.WriteInt32(it->alignment.maxVal);
        (void)parcel.WriteInt32(it->width.minVal);
        (void)parcel.WriteInt32(it->width.maxVal);
        (void)parcel.WriteInt32(it->height.minVal);
        (void)parcel.WriteInt32(it->height.maxVal);
        (void)parcel.WriteInt32(it->frameRate.minVal);
        (void)parcel.WriteInt32(it->frameRate.maxVal);
        (void)parcel.WriteInt32(it->encodeQuality.minVal);
        (void)parcel.WriteInt32(it->encodeQuality.maxVal);
        (void)parcel.WriteInt32(it->quality.minVal);
        (void)parcel.WriteInt32(it->quality.maxVal);
        (void)parcel.WriteInt32(it->blockPerFrame.minVal);
        (void)parcel.WriteInt32(it->blockPerFrame.maxVal);
        (void)parcel.WriteInt32(it->blockPerSecond.minVal);
        (void)parcel.WriteInt32(it->blockPerSecond.maxVal);
        (void)parcel.WriteInt32(it->blockSize.width);
        (void)parcel.WriteInt32(it->blockSize.height);
        (void)parcel.WriteInt32Vector(it->sampleRate);
        (void)parcel.WriteInt32Vector(it->format);
        (void)parcel.WriteInt32Vector(it->profiles);
        (void)parcel.WriteInt32Vector(it->bitrateMode);
        (void)parcel.WriteInt32Vector(it->levels);
        (void)Marshalling(parcel, it->measuredFrameRate);
        (void)Marshalling(parcel, it->profileLevelsMap);
    }
    MEDIA_LOGD("success to Marshalling capabilityDataArray");

    return true;
}

bool AVCodecListParcel::Marshalling(MessageParcel &parcel, const std::map<ImgSize, Range> &mapSizeToRange)
{
    parcel.WriteUint32(mapSizeToRange.size());
    for (auto it = mapSizeToRange.begin(); it != mapSizeToRange.end(); it++) {
        (void)parcel.WriteInt32(it->first.width);
        (void)parcel.WriteInt32(it->first.height);
        (void)parcel.WriteInt32(it->second.minVal);
        (void)parcel.WriteInt32(it->second.maxVal);
    }
    return true;
}

bool AVCodecListParcel::Marshalling(MessageParcel &parcel, const std::map<int32_t, std::vector<int32_t>> &mapIntToVec)
{
    parcel.WriteUint32(mapIntToVec.size());
    for (auto it = mapIntToVec.begin(); it != mapIntToVec.end(); it++) {
        (void)parcel.WriteInt32(it->first);
        (void)parcel.WriteInt32Vector(it->second);
    }
    return true;
}

bool AVCodecListParcel::Unmarshalling(MessageParcel &parcel, std::vector<CapabilityData> &capabilityDataArray)
{
    uint32_t size = parcel.ReadUint32();
    for (uint32_t index = 0; index < size; index++) {
        CapabilityData capabilityData;
        capabilityData.codecName = parcel.ReadString();
        capabilityData.mimeType = parcel.ReadString();
        capabilityData.isVendor = parcel.ReadBool();
        capabilityData.codecType = parcel.ReadInt32();
        capabilityData.bitrate.minVal = parcel.ReadInt32();
        capabilityData.bitrate.maxVal = parcel.ReadInt32();
        capabilityData.channels.minVal = parcel.ReadInt32();
        capabilityData.channels.maxVal = parcel.ReadInt32();
        capabilityData.complexity.minVal = parcel.ReadInt32();
        capabilityData.complexity.maxVal = parcel.ReadInt32();
        capabilityData.alignment.minVal = parcel.ReadInt32();
        capabilityData.alignment.maxVal = parcel.ReadInt32();
        capabilityData.width.minVal = parcel.ReadInt32();
        capabilityData.width.maxVal = parcel.ReadInt32();
        capabilityData.height.minVal = parcel.ReadInt32();
        capabilityData.height.maxVal = parcel.ReadInt32();
        capabilityData.frameRate.minVal = parcel.ReadInt32();
        capabilityData.frameRate.maxVal = parcel.ReadInt32();
        capabilityData.encodeQuality.minVal = parcel.ReadInt32();
        capabilityData.encodeQuality.maxVal = parcel.ReadInt32();
        capabilityData.quality.minVal = parcel.ReadInt32();
        capabilityData.quality.maxVal = parcel.ReadInt32();
        capabilityData.blockPerFrame.minVal = parcel.ReadInt32();
        capabilityData.blockPerFrame.maxVal = parcel.ReadInt32();
        capabilityData.blockPerSecond.minVal = parcel.ReadInt32();
        capabilityData.blockPerSecond.maxVal = parcel.ReadInt32();
        capabilityData.blockSize.width = parcel.ReadInt32();
        capabilityData.blockSize.height = parcel.ReadInt32();
        parcel.ReadInt32Vector(&capabilityData.sampleRate);
        parcel.ReadInt32Vector(&capabilityData.format);
        parcel.ReadInt32Vector(&capabilityData.profiles);
        parcel.ReadInt32Vector(&capabilityData.bitrateMode);
        parcel.ReadInt32Vector(&capabilityData.levels);
        Unmarshalling(parcel, capabilityData.measuredFrameRate);
        Unmarshalling(parcel, capabilityData.profileLevelsMap);
        capabilityDataArray.push_back(capabilityData);
    }
    MEDIA_LOGD("success to Unmarshalling capabilityDataArray");

    return true;
}

bool AVCodecListParcel::Unmarshalling(MessageParcel &parcel, std::map<ImgSize, Range> &mapSizeToRange)
{
    uint32_t size = parcel.ReadUint32();
    for (uint32_t index = 0; index < size; index++) {
        ImgSize key;
        Range values;
        key.width = parcel.ReadInt32();
        key.height = parcel.ReadInt32();
        values.minVal = parcel.ReadInt32();
        values.maxVal = parcel.ReadInt32();
        mapSizeToRange.insert(std::make_pair(key, values));
    }
    return true;
}

bool AVCodecListParcel::Unmarshalling(MessageParcel &parcel, std::map<int32_t, std::vector<int32_t>> &mapIntToVec)
{
    uint32_t size = parcel.ReadUint32();
    for (uint32_t index = 0; index < size; index++) {
        int32_t key;
        std::vector<int32_t> values;
        key = parcel.ReadInt32();
        parcel.ReadInt32Vector(&values);
        mapIntToVec.insert(std::make_pair(key, values));
    }
    return true;
}
} // namespace Media
} // namespace OHOS
