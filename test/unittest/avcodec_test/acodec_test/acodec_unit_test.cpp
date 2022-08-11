/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"
#include "media_errors.h"
#include "acodec_unit_test.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace OHOS::Media::ACodecTestParam;

void AudioCodecUnitTest::SetUpTestCase(void) {}

void AudioCodecUnitTest::TearDownTestCase(void) {}

void AudioCodecUnitTest::SetUp(void)
{
    std::shared_ptr<ACodecSignal> acodecSignal = std::make_shared<ACodecSignal>();
    adecCallback_ = std::make_shared<ADecCallbackTest>(acodecSignal);
    ASSERT_NE(nullptr, adecCallback_);
    audioCodec_ = std::make_shared<ACodecMock>(acodecSignal);
    ASSERT_NE(nullptr, audioCodec_);
    if (createByMineFlag_) {
        ASSERT_TRUE(audioCodec_->CreateAudioDecMockByMine("audio/mp4a-latm"));
    } else {
        ASSERT_TRUE(audioCodec_->CreateAudioDecMockByName("avdec_aac"));
    }
    EXPECT_EQ(MSERR_OK, audioCodec_->SetCallbackDec(adecCallback_));

    aencCallback_ = std::make_shared<AEncCallbackTest>(acodecSignal);
    ASSERT_NE(nullptr, aencCallback_);
    if (createByMineFlag_) {
        ASSERT_TRUE(audioCodec_->CreateAudioEncMockByMine("audio/mp4a-latm"));
    } else {
        ASSERT_TRUE(audioCodec_->CreateAudioEncMockByName("avenc_aac"));
    }
    EXPECT_EQ(MSERR_OK, audioCodec_->SetCallbackEnc(aencCallback_));
    testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    string suffix = ".es";
    audioCodec_->SetOutPath(prefix + fileName + suffix);
}

void AudioCodecUnitTest::TearDown(void)
{
    if (audioCodec_ != nullptr) {
        EXPECT_EQ(MSERR_OK, audioCodec_->ReleaseDec());
        EXPECT_EQ(MSERR_OK, audioCodec_->ReleaseEnc());
    }
}

/**
 * @tc.name: audio_codec_Configure_0100
 * @tc.desc: video create
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AudioCodecUnitTest, audio_codec_Configure_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    (void)format->PutIntValue("channel_count", 2);
    (void)format->PutIntValue("sample_rate", 44100);
    (void)format->PutIntValue("audio_sample_format", 1);
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(format));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(format));
}

/**
 * @tc.name: audio_codec_0100
 * @tc.desc: audio decodec h264->mpeg4
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AudioCodecUnitTest, audio_codec_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    (void)format->PutIntValue("channel_count", 2);
    (void)format->PutIntValue("sample_rate", 44100);
    (void)format->PutIntValue("audio_sample_format", 1);
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(format));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(format));
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareEnc());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartEnc());
    sleep(10); // start run 10s
    EXPECT_EQ(MSERR_OK, audioCodec_->StopDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->StopEnc());
}

/**
 * @tc.name: audio_decodec_flush_0100
 * @tc.desc: audio decodec flush
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AudioCodecUnitTest, audio_decodec_flush_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    (void)format->PutIntValue("channel_count", 2);
    (void)format->PutIntValue("sample_rate", 44100);
    (void)format->PutIntValue("audio_sample_format", 1);
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(format));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(format));
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareEnc());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartEnc());
    sleep(3); // start run 2s
    EXPECT_EQ(MSERR_OK, audioCodec_->FlushDec());
    sleep(7); // start run 7s
    EXPECT_EQ(MSERR_OK, audioCodec_->StopDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->StopEnc());
}

/**
 * @tc.name: audio_encodec_flush_0100
 * @tc.desc: audio encodec flush
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AudioCodecUnitTest, audio_encodec_flush_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    (void)format->PutIntValue("channel_count", 2);
    (void)format->PutIntValue("sample_rate", 44100);
    (void)format->PutIntValue("audio_sample_format", 1);
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(format));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(format));
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareEnc());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartEnc());
    sleep(3); // start run 2s
    EXPECT_EQ(MSERR_OK, audioCodec_->FlushEnc());
    sleep(7); // start run 7s
    EXPECT_EQ(MSERR_OK, audioCodec_->StopDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->StopEnc());
}

/**
 * @tc.name: audio_codec_reset_0100
 * @tc.desc: audio reset at end of stream
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AudioCodecUnitTest, audio_codec_reset_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    (void)format->PutIntValue("channel_count", 2);
    (void)format->PutIntValue("sample_rate", 44100);
    (void)format->PutIntValue("audio_sample_format", 1);
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(format));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(format));
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareEnc());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartEnc());
    sleep(10); // start run 10s
    EXPECT_EQ(MSERR_OK, audioCodec_->ResetDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->ResetEnc());
}

/**
 * @tc.name: audio_codec_reset_0200
 * @tc.desc: audio reset at running state
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AudioCodecUnitTest, audio_codec_reset_0200, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    (void)format->PutIntValue("channel_count", 2);
    (void)format->PutIntValue("sample_rate", 44100);
    (void)format->PutIntValue("audio_sample_format", 1);
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(format));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(format));
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareEnc());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartEnc());
    sleep(2); // start run 10s
    EXPECT_EQ(MSERR_OK, audioCodec_->ResetDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->ResetEnc());
}

/**
 * @tc.name: audio_codec_SetParameter_0100
 * @tc.desc: audio codec SetParameter
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AudioCodecUnitTest, audio_codec_SetParameter_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    (void)format->PutIntValue("channel_count", 2);
    (void)format->PutIntValue("sample_rate", 44100);
    (void)format->PutIntValue("audio_sample_format", 1);
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(format));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(format));
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareEnc());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartEnc());
    sleep(2); // start run 2s
    EXPECT_EQ(MSERR_OK, audioCodec_->SetParameterDec(format));
    EXPECT_EQ(MSERR_OK, audioCodec_->SetParameterEnc(format));
    EXPECT_EQ(MSERR_OK, audioCodec_->ResetDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->ResetEnc());
}

/**
 * @tc.name: audio_codec_GetOutputMediaDescription_0100
 * @tc.desc: audio codec GetOutputMediaDescription
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(AudioCodecUnitTest, audio_codec_GetOutputMediaDescription_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    (void)format->PutIntValue("channel_count", 2);
    (void)format->PutIntValue("sample_rate", 44100);
    (void)format->PutIntValue("audio_sample_format", 1);
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(format));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(format));
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareEnc());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartEnc());
    sleep(2); // start run 2s
    EXPECT_NE(nullptr, audioCodec_->GetOutputMediaDescriptionDec());
    EXPECT_NE(nullptr, audioCodec_->GetOutputMediaDescriptionEnc());
    EXPECT_EQ(MSERR_OK, audioCodec_->ResetDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->ResetEnc());
}
