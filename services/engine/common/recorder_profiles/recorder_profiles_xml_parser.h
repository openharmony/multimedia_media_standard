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

#ifndef RECORDERPROFILES_XML_PARSER_H
#define RECORDERPROFILES_XML_PARSER_H

#include "xml_parse.h"
#include "i_recorder_profiles_service.h"

namespace OHOS {
namespace Media {
enum class RecorderProfilesNodeName : int32_t {
    RECORDER_CONFIGURATIONS,
    RECORDER_CAPS,
    RECORDER_PROFILES,
    UNKNOWN,
};

struct ContainerFormatInfo {
    std::string name = "";
    std::string hasVideo = "";
};

class RecorderProfilesXmlParser : public XmlParser {
public:
    RecorderProfilesXmlParser();
    ~RecorderProfilesXmlParser();
    std::vector<RecorderProfilesData> GetRecorderProfileDataArray();

private:
    bool ParseInternal(xmlNode *node) override;
    bool SetCapabilityIntData(std::unordered_map<std::string, int32_t&> dataMap,
                                const std::string &capabilityKey, const std::string &capabilityValue) const override;
    bool SetCapabilityVectorData(std::unordered_map<std::string, std::vector<int32_t>&> dataMap,
                                const std::string &capabilityKey, const std::string &capabilityValue) const override;
    RecorderProfilesNodeName GetNodeNameAsInt(xmlNode *node);
    bool SetVideoRecorderProfiles(RecorderProfilesData &data, const std::string &capabilityKey,
        const std::string &capabilityValue);
    bool SetAudioRecorderProfiles(RecorderProfilesData &data, const std::string &capabilityKey,
        const std::string &capabilityValue);
    bool SetVideoRecorderCaps(RecorderProfilesData &data, const std::string &capabilityKey,
        const std::string &capabilityValue);
    bool SetAudioRecorderCaps(RecorderProfilesData &data, const std::string &capabilityKey,
        const std::string &capabilityValue);
    bool SetContainerFormat(ContainerFormatInfo &data, const std::string &capabilityKey,
        const std::string &capabilityValue);
    bool ParseRecorderCapsData(xmlNode *node);
    bool ParseRecorderContainerFormatData(xmlNode *node);
    bool ParseRecorderEncodeCapsData(xmlNode *node, bool isVideo);
    bool ParseRecorderProfilesData(xmlNode *node);
    bool ParseRecorderProfilesSourceData(const std::string sourceType, xmlNode *node);
    bool ParseRecorderProfileSettingsData(xmlNode *node, RecorderProfilesData &capabilityData);
    bool ParseRecorderProfileVideoAudioData(xmlNode *node, RecorderProfilesData &capabilityData);
    void PackageRecorderCaps();
    void PackageVideoRecorderCaps(const std::string &formatType);
    void PackageAudioRecorderCaps(const std::string &formatType);
    void PaddingVideoCapsByAudioCaps(const std::string &formatType, RecorderProfilesData &videoData);
    bool ParseAudioRecorderProfiles(
        xmlNode *node, RecorderProfilesData &capabilityData, const std::string &capabilityKey);
    bool ParseVideoRecorderProfiles(
        xmlNode *node, RecorderProfilesData &capabilityData, const std::string &capabilityKey);
    bool ParseVideoRecorderProfilesForVideoAudioData(
        xmlNode *node, RecorderProfilesData &capabilityData, const std::string &capabilityKey);

    std::vector<ContainerFormatInfo> containerFormatArray_;
    std::vector<RecorderProfilesData> videoEncoderCapsArray_;
    std::vector<RecorderProfilesData> audioEncoderCapsArray_;
    std::vector<RecorderProfilesData> capabilityDataArray_;
    std::vector<std::string> capabilityKeys_;
};
}  // namespace Media
}  // namespace OHOS
#endif  // RECORDERPROFILES_XML_PARSER_H