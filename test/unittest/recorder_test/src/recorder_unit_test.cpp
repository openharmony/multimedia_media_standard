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

#include "recorder_unit_test.h"
#include "media_errors.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;
using namespace OHOS::Media::RecorderTestParam;

// config for video to request buffer from surface
static VideoRecorderConfig g_videoRecorderConfig;

void RecorderUnitTest::SetUpTestCase(void) {}
void RecorderUnitTest::TearDownTestCase(void) {}

void RecorderUnitTest::SetUp(void)
{
    recorder_ = std::make_shared<RecorderMock>();
    ASSERT_NE(nullptr, recorder_);
    ASSERT_TRUE(recorder_->CreateRecorder());
}

void RecorderUnitTest::TearDown(void)
{
    if (recorder_ != nullptr) {
        recorder_->Release();
    }
}


/**
 * @tc.name: recorder_video_yuv_mpeg4
 * @tc.desc: recorde video with yuv mpeg4
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_yuv_mpeg4, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/recorder_video_yuv_mpeg4.mp4", O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}


/**
 * @tc.name: recorder_audio_es
 * @tc.desc: recorde audio with es
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_audio_es, TestSize.Level0)
{
    g_videoRecorderConfig.outputFd = open("/data/test/recorder_audio_es.m4a", O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_AUDIO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_av_yuv_mpeg4
 * @tc.desc: recorde audio with yuv mpeg4
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_av_yuv_mpeg4, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/recorder_av_yuv_mpeg4.mp4", O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(AUDIO_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(AUDIO_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_es
 * @tc.desc: recorde video ,then pause resume
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_pause_resume, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/recorder_video_pause_resume.mp4", O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Pause());
    sleep(RECORDER_TIME / 2);
    EXPECT_EQ(MSERR_OK, recorder_->Resume());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetOrientationHint_001
 * @tc.desc: recorde video ,SetOrientationHint
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetOrientationHint_001, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/recorder_video_SetOrientationHint_001.mp4", O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);

    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetLocation(0.0, 0);
    recorder_->SetOrientationHint(90);
    EXPECT_EQ(MSERR_OK, recorder_->Prepare());
    EXPECT_EQ(MSERR_OK, recorder_->RequesetBuffer(PURE_VIDEO, g_videoRecorderConfig));

    EXPECT_EQ(MSERR_OK, recorder_->Start());
    sleep(RECORDER_TIME);
    EXPECT_EQ(MSERR_OK, recorder_->Stop(false));
    recorder_->StopBuffer(PURE_VIDEO);
    EXPECT_EQ(MSERR_OK, recorder_->Reset());
    EXPECT_EQ(MSERR_OK, recorder_->Release());
    close(g_videoRecorderConfig.outputFd);
}

/**
 * @tc.name: recorder_video_SetMaxFileSize_001
 * @tc.desc: recorde video ,SetMaxFileSize
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(RecorderUnitTest, recorder_video_SetMaxFileSize_001, TestSize.Level0)
{
    g_videoRecorderConfig.vSource = VIDEO_SOURCE_SURFACE_YUV;
    g_videoRecorderConfig.videoFormat = MPEG4;
    g_videoRecorderConfig.outputFd = open("/data/test/recorder_video_SetMaxFileSize_001.mp4", O_RDWR);
    ASSERT_TRUE(g_videoRecorderConfig.outputFd >= 0);
    EXPECT_EQ(MSERR_OK, recorder_->SetFormat(PURE_VIDEO, g_videoRecorderConfig));
    recorder_->SetCaptureRate(0, 30);
    EXPECT_EQ(MSERR_OK, recorder_->SetMaxFileSize(5000));
    EXPECT_EQ(MSERR_OK, recorder_->SetNextOutputFile(g_videoRecorderConfig.outputFd));
    recorder_->SetFileSplitDuration(FileSplitType::FILE_SPLIT_POST, -1, 1000);
    close(g_videoRecorderConfig.outputFd);
}