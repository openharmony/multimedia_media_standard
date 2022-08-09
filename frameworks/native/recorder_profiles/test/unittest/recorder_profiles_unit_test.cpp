/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "recorder_profiles_unit_test.h"
#include "avcodec_info.h"
#include "avcontainer_common.h"
#include "media_errors.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
void RecorderProfilesUnitTest::SetUpTestCase(void) {}
void RecorderProfilesUnitTest::TearDownTestCase(void) {}

void RecorderProfilesUnitTest::SetUp(void)
{
}

void RecorderProfilesUnitTest::TearDown(void)
{
}

/**
 * @tc.name: recorder_profile_IsAudioRecoderConfigSupported_0100
 * @tc.desc: recorde profile IsAudioRecoderConfigSupported
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderProfilesUnitTest, recorder_profile_IsAudioRecoderConfigSupported_0100, TestSize.Level0)
{
    std::shared_ptr<AudioRecorderProfile> profile  = std::make_shared<AudioRecorderProfile>();
    profile->containerFormatType = ContainerFormatType::CFT_MPEG_4;
    profile->audioCodec = CodecMimeType::AUDIO_AAC;
    profile->audioBitrate = 96000;
    profile->audioSampleRate = 48000;
    profile->audioChannels = 2;
    EXPECT_TRUE(RecorderProfilesFactory::CreateRecorderProfiles().IsAudioRecoderConfigSupported(*profile));
}

/**
 * @tc.name: recorder_profile_HasVideoRecorderProfile_0100
 * @tc.desc: recorde profile HasVideoRecorderProfile
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderProfilesUnitTest, recorder_profile_HasVideoRecorderProfile_0100, TestSize.Level0)
{
    int32_t sourceId = 0;
    int32_t qualityLevel = RECORDER_QUALITY_LOW;
    EXPECT_TRUE(RecorderProfilesFactory::CreateRecorderProfiles().HasVideoRecorderProfile(sourceId, qualityLevel));
}

bool RecorderProfilesUnitTest::CheckAudioRecorderCapsArray(
    const std::vector<std::shared_ptr<AudioRecorderCaps>> &audioRecorderArray) const
{
    bool flagM4a = false;
    bool flagAAC = false;
    for (auto iter = audioRecorderArray.begin(); iter != audioRecorderArray.end(); iter++) {
        std::shared_ptr<AudioRecorderCaps> pAudioRecorderCaps = *iter;
        if ((pAudioRecorderCaps->containerFormatType.compare(ContainerFormatType::CFT_MPEG_4A) == 0)) {
            flagM4a = true;
        }
        if ((pAudioRecorderCaps->mimeType.compare(CodecMimeType::AUDIO_AAC) == 0)) {
            flagAAC = true;
            EXPECT_GE(pAudioRecorderCaps->bitrate.minVal, 0);
            EXPECT_GE(pAudioRecorderCaps->channels.minVal, 0);
            EXPECT_GE(pAudioRecorderCaps->sampleRate.size(), 0);
        }
    }
    return flagM4a && flagAAC;
}

bool RecorderProfilesUnitTest::CheckVideoRecorderCapsArray(
    const std::vector<std::shared_ptr<VideoRecorderCaps>> &videoRecorderArray) const
{
    bool flagMP4 = false;
    bool flagMP4A = false;
    bool flagMP4V = false;
    bool flagAVC = false;
    for (auto iter =  videoRecorderArray.begin(); iter !=  videoRecorderArray.end(); iter++) {
        std::shared_ptr< VideoRecorderCaps> pVideoRecorderCaps = *iter;
        if ((pVideoRecorderCaps->containerFormatType.compare(ContainerFormatType::CFT_MPEG_4) == 0)) {
            flagMP4 = true;
        }
        if ((pVideoRecorderCaps->audioEncoderMime.compare(CodecMimeType::AUDIO_AAC) == 0)) {
            flagMP4A = true;
        }
        if ((pVideoRecorderCaps->videoEncoderMime.compare(CodecMimeType::VIDEO_MPEG4) == 0)) {
            flagMP4V = true;
            EXPECT_GE(pVideoRecorderCaps->audioBitrateRange.minVal, 0);
            EXPECT_GE(pVideoRecorderCaps->audioChannelRange.minVal, 0);
            EXPECT_GE(pVideoRecorderCaps->videoBitrateRange.minVal, 0);
            EXPECT_GE(pVideoRecorderCaps->videoFramerateRange.minVal, 0);
            EXPECT_GE(pVideoRecorderCaps->videoWidthRange.minVal, 0);
            EXPECT_GE(pVideoRecorderCaps->videoHeightRange.minVal, 0);
            EXPECT_GE(pVideoRecorderCaps->audioSampleRates.size(), 0);
        }
        if ((pVideoRecorderCaps->videoEncoderMime.compare(CodecMimeType::VIDEO_AVC) == 0)) {
            flagAVC = true;
        }
    }
    return flagMP4 && flagMP4A && flagMP4V && flagAVC;
}

/**
 * @tc.name: recorder_profile_GetAudioRecorderCaps_0100
 * @tc.desc: recorde profile GetAudioRecorderCaps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderProfilesUnitTest, recorder_profile_GetAudioRecorderCaps_0100, TestSize.Level0)
{
    std::vector<std::shared_ptr<AudioRecorderCaps>> audioRecorderArray =
        RecorderProfilesFactory::CreateRecorderProfiles().GetAudioRecorderCaps();
    EXPECT_TRUE(CheckAudioRecorderCapsArray(audioRecorderArray));
}

/**
 * @tc.name: recorder_profile_GetVideoRecorderCaps_0100
 * @tc.desc: recorde profile GetVideoRecorderCaps
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderProfilesUnitTest, recorder_profile_GetVideoRecorderCaps_0100, TestSize.Level0)
{
    std::vector<std::shared_ptr<VideoRecorderCaps>> videoRecorderArray =
        RecorderProfilesFactory::CreateRecorderProfiles().GetVideoRecorderCaps();
    CheckVideoRecorderCapsArray(videoRecorderArray);
}

/**
 * @tc.name: recorder_profile_GetVideoRecorderProfile_0100
 * @tc.desc: recorde profile GetVideoRecorderProfile
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderProfilesUnitTest, recorder_profile_GetVideoRecorderProfile_0100, TestSize.Level0)
{
    int32_t sourceId = 0;
    int32_t qualityLevel = RECORDER_QUALITY_LOW;
    std::shared_ptr<VideoRecorderProfile> videoRecorderProfile =
        RecorderProfilesFactory::CreateRecorderProfiles().GetVideoRecorderProfile(sourceId, qualityLevel);
    EXPECT_EQ(ContainerFormatType::CFT_MPEG_4, videoRecorderProfile->containerFormatType);
    EXPECT_EQ(96000, videoRecorderProfile->audioBitrate);
    EXPECT_EQ(2, videoRecorderProfile->audioChannels);
    EXPECT_EQ(CodecMimeType::AUDIO_AAC, videoRecorderProfile->audioCodec);
    EXPECT_EQ(48000, videoRecorderProfile->audioSampleRate);
    EXPECT_EQ(30, videoRecorderProfile->durationTime);
    EXPECT_EQ(RECORDER_QUALITY_LOW, videoRecorderProfile->qualityLevel);
    EXPECT_EQ(192000, videoRecorderProfile->videoBitrate);
    EXPECT_EQ(CodecMimeType::VIDEO_MPEG4, videoRecorderProfile->videoCodec);
    EXPECT_EQ(176, videoRecorderProfile->videoFrameWidth);
    EXPECT_EQ(144, videoRecorderProfile->videoFrameHeight);
    EXPECT_EQ(30, videoRecorderProfile->videoFrameRate);
}
} // namespace Media
} // namespace OHOS