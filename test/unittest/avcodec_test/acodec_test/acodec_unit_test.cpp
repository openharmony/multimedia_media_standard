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

void ACodecUnitTest::SetUpTestCase(void) {}

void ACodecUnitTest::TearDownTestCase(void) {}

void ACodecUnitTest::SetUp(void)
{
    std::shared_ptr<ACodecSignal> acodecSignal = std::make_shared<ACodecSignal>();
    adecCallback_ = std::make_shared<ADecCallbackTest>(acodecSignal);
    ASSERT_NE(nullptr, adecCallback_);
    audioCodec_ = std::make_shared<ACodecMock>(acodecSignal);
    ASSERT_NE(nullptr, audioCodec_);
    if (createByMimeFlag_) {
        ASSERT_TRUE(audioCodec_->CreateAudioDecMockByMime("audio/mp4a-latm"));
    } else {
        ASSERT_TRUE(audioCodec_->CreateAudioDecMockByName("avdec_aac"));
    }
    EXPECT_EQ(MSERR_OK, audioCodec_->SetCallbackDec(adecCallback_));

    aencCallback_ = std::make_shared<AEncCallbackTest>(acodecSignal);
    ASSERT_NE(nullptr, aencCallback_);
    if (createByMimeFlag_) {
        ASSERT_TRUE(audioCodec_->CreateAudioEncMockByMime("audio/mp4a-latm"));
    } else {
        ASSERT_TRUE(audioCodec_->CreateAudioEncMockByName("avenc_aac"));
    }
    EXPECT_EQ(MSERR_OK, audioCodec_->SetCallbackEnc(aencCallback_));

    defaultFormat_ = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, defaultFormat_);
    (void)defaultFormat_->PutIntValue("channel_count", 2); // 2 common channel count
    (void)defaultFormat_->PutIntValue("sample_rate", 44100); // 44100 common sample rate
    (void)defaultFormat_->PutIntValue("audio_sample_format", 1); // 1 AudioStandard::SAMPLE_S16LE

    testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    string suffix = ".es";
    audioCodec_->SetOutPath(prefix + fileName + suffix);
}

void ACodecUnitTest::TearDown(void)
{
    if (audioCodec_ != nullptr) {
        EXPECT_EQ(MSERR_OK, audioCodec_->ReleaseDec());
        EXPECT_EQ(MSERR_OK, audioCodec_->ReleaseEnc());
    }
    if (defaultFormat_ != nullptr) {
        defaultFormat_->Destroy();
    }
}

/**
 * @tc.name: audio_codec_Configure_0100
 * @tc.desc: video create
 * @tc.type: FUNC
 * @tc.require: I5OWXY I5OXCD
 */
HWTEST_F(ACodecUnitTest, audio_codec_Configure_0100, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(defaultFormat_));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(defaultFormat_));
}

/**
 * @tc.name: audio_codec_0100
 * @tc.desc: audio decodec h264->mpeg4
 * @tc.type: FUNC
 * @tc.require: I5OWXY I5OXCD
 */
HWTEST_F(ACodecUnitTest, audio_codec_0100, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(defaultFormat_));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(defaultFormat_));
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
 * @tc.require: I5OWXY I5OXCD
 */
HWTEST_F(ACodecUnitTest, audio_decodec_flush_0100, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(defaultFormat_));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(defaultFormat_));
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
 * @tc.require: I5OWXY I5OXCD
 */
HWTEST_F(ACodecUnitTest, audio_encodec_flush_0100, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(defaultFormat_));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(defaultFormat_));
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
 * @tc.require: I5OWXY I5OXCD
 */
HWTEST_F(ACodecUnitTest, audio_codec_reset_0100, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(defaultFormat_));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(defaultFormat_));
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
 * @tc.require: I5OWXY I5OXCD
 */
HWTEST_F(ACodecUnitTest, audio_codec_reset_0200, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(defaultFormat_));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(defaultFormat_));
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
 * @tc.require: I5OWXY I5OXCD
 */
HWTEST_F(ACodecUnitTest, audio_codec_SetParameter_0100, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(defaultFormat_));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(defaultFormat_));
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->PrepareEnc());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->StartEnc());
    sleep(2); // start run 2s
    EXPECT_EQ(MSERR_OK, audioCodec_->SetParameterDec(defaultFormat_));
    EXPECT_EQ(MSERR_OK, audioCodec_->SetParameterEnc(defaultFormat_));
    EXPECT_EQ(MSERR_OK, audioCodec_->ResetDec());
    EXPECT_EQ(MSERR_OK, audioCodec_->ResetEnc());
}

/**
 * @tc.name: audio_codec_GetOutputMediaDescription_0100
 * @tc.desc: audio codec GetOutputMediaDescription
 * @tc.type: FUNC
 * @tc.require: I5OWXY I5OXCD
 */
HWTEST_F(ACodecUnitTest, audio_codec_GetOutputMediaDescription_0100, TestSize.Level0)
{
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureEnc(defaultFormat_));
    ASSERT_EQ(MSERR_OK, audioCodec_->ConfigureDec(defaultFormat_));
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
