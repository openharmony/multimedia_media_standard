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
#include "vcodec_unit_test.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace OHOS::Media::VCodecTestParam;

void VCodecUnitTest::SetUpTestCase(void) {}

void VCodecUnitTest::TearDownTestCase(void) {}

void VCodecUnitTest::SetUp(void)
{
    std::shared_ptr<VDecSignal> vdecSignal = std::make_shared<VDecSignal>();
    vdecCallback_ = std::make_shared<VDecCallbackTest>(vdecSignal);
    ASSERT_NE(nullptr, vdecCallback_);
    videoDec_ = std::make_shared<VDecMock>(vdecSignal);
    ASSERT_TRUE(videoDec_->CreateVideoDecMockByMime("video/avc"));
    EXPECT_EQ(MSERR_OK, videoDec_->SetCallback(vdecCallback_));

    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    vencCallback_ = std::make_shared<VEncCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencCallback_);
    videoEnc_ = std::make_shared<VEncMock>(vencSignal);
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByMime("video/avc"));
    EXPECT_EQ(MSERR_OK, videoEnc_->SetCallback(vencCallback_));

    testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    string suffix = ".es";
    videoEnc_->SetOutPath(prefix + fileName + suffix);
    createCodecSuccess_ = true;
}

bool VCodecUnitTest::CreateVideoCodecByName(const std::string &decName, const std::string &encName)
{
    std::shared_ptr<VDecSignal> vdecSignal = std::make_shared<VDecSignal>();
    vdecCallback_ = std::make_shared<VDecCallbackTest>(vdecSignal);
    videoDec_ = std::make_shared<VDecMock>(vdecSignal);
    if (!videoDec_->CreateVideoDecMockByName(decName) || videoDec_->SetCallback(vdecCallback_) != MSERR_OK) {
        return false;
    }

    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    vencCallback_ = std::make_shared<VEncCallbackTest>(vencSignal);
    videoEnc_ = std::make_shared<VEncMock>(vencSignal);
    if (!videoEnc_->CreateVideoEncMockByName(encName) || videoEnc_->SetCallback(vencCallback_) != MSERR_OK) {
        return false;
    }
    testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    string suffix = ".es";
    videoEnc_->SetOutPath(prefix + fileName + suffix);
    createCodecSuccess_ = true;
    return true;
}

void VCodecUnitTest::TearDown(void)
{
    if (videoDec_ != nullptr && createCodecSuccess_) {
        EXPECT_EQ(MSERR_OK, videoDec_->Reset());
        EXPECT_EQ(MSERR_OK, videoDec_->Release());
    }

    if (videoEnc_ != nullptr && createCodecSuccess_) {
        EXPECT_EQ(MSERR_OK, videoEnc_->Reset());
        EXPECT_EQ(MSERR_OK, videoEnc_->Release());
    }
}

/**
 * @tc.name: video_codec_creat_0100
 * @tc.desc: video create
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(VCodecUnitTest, video_codec_creat_0100, TestSize.Level0)
{
    if (videoDec_ != nullptr) {
        EXPECT_EQ(MSERR_OK, videoDec_->Release());
    }
    if (videoEnc_ != nullptr) {
        EXPECT_EQ(MSERR_OK, videoEnc_->Release());
    }
    ASSERT_TRUE(videoDec_->CreateVideoDecMockByName("avdec_h264"));
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName("avenc_mpeg4"));
}

/**
 * @tc.name: video_codec_Configure_0100
 * @tc.desc: video codec Configure
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(VCodecUnitTest, video_codec_Configure_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    format->Destroy();
}

/**
 * @tc.name: video_codec_start_0100
 * @tc.desc: video decodec start
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(VCodecUnitTest, video_codec_start_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    format->Destroy();
}

/**
 * @tc.name: video_codec_0100
 * @tc.desc: video decodec h264->mpeg4
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(VCodecUnitTest, video_codec_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(10); // start run 10s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_codec_0200
 * @tc.desc: video codec h265->h265
 * @tc.type: FUNC
 * @tc.require: I5OOKW I5OOKN
 */
HWTEST_F(VCodecUnitTest, video_codec_0200, TestSize.Level0)
{    
    if (videoDec_ != nullptr) {
        EXPECT_EQ(MSERR_OK, videoDec_->Release());
    }
    if (videoEnc_ != nullptr) {
        EXPECT_EQ(MSERR_OK, videoEnc_->Release());
    }
    if (!CreateVideoCodecByName("OMX_hisi_video_decoder_hevc", "OMX_hisi_video_encoder_hevc")) {
        std::cout << "This device does not support hard hevc" << std::endl;
        createCodecSuccess_ = false;
        return;
    }
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H265_SRC_PATH, ES_H265, ES_LENGTH_H265);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(10); // start run 10s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_decode_Flush_0100
 * @tc.desc: video decodec flush
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(VCodecUnitTest, video_decode_Flush_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(3); // start run 3s
    EXPECT_EQ(MSERR_OK, videoDec_->Flush());
    sleep(7); // start run 7s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_encode_Flush_0100
 * @tc.desc: video encodec flush
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(VCodecUnitTest, video_encode_Flush_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(3); // start run 3s
    EXPECT_EQ(MSERR_OK, videoEnc_->Flush());
    sleep(7); // start run 7s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_codec_SetParameter_0100
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(VCodecUnitTest, video_codec_SetParameter_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));
    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->SetParameter(format));
    EXPECT_EQ(MSERR_OK, videoDec_->SetParameter(format));
    sleep(5); // start run 5s
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_codec_GetOutputMediaDescription_0100
 * @tc.desc: video codec GetOutputMediaDescription
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(VCodecUnitTest, video_codec_GetOutputMediaDescription_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));
    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(2); // start run 2s
    EXPECT_NE(nullptr, videoDec_->GetOutputMediaDescription());
    EXPECT_NE(nullptr, videoEnc_->GetOutputMediaDescription());
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}

/**
 * @tc.name: video_NotifyEos_0100
 * @tc.desc: video encodec NotifyEos
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(VCodecUnitTest, video_NotifyEos_0100, TestSize.Level0)
{
    std::shared_ptr<FormatMock> format = AVCodecMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format);
    string width = "width";
    string height = "height";
    string pixelFormat = "pixel_format";
    string frame_rate = "frame_rate";
    (void)format->PutIntValue(width.c_str(), DEFAULT_WIDTH);
    (void)format->PutIntValue(height.c_str(), DEFAULT_HEIGHT);
    (void)format->PutIntValue(pixelFormat.c_str(), NV12);
    (void)format->PutIntValue(frame_rate.c_str(), DEFAULT_FRAME_RATE);
    videoDec_->SetSource(H264_SRC_PATH, ES_H264, ES_LENGTH_H264);
    ASSERT_EQ(MSERR_OK, videoEnc_->Configure(format));
    ASSERT_EQ(MSERR_OK, videoDec_->Configure(format));
    std::shared_ptr<SurfaceMock> surface = videoEnc_->GetInputSurface();
    ASSERT_NE(nullptr, surface);
    ASSERT_EQ(MSERR_OK, videoDec_->SetOutputSurface(surface));

    EXPECT_EQ(MSERR_OK, videoDec_->Prepare());
    EXPECT_EQ(MSERR_OK, videoEnc_->Prepare());
    EXPECT_EQ(MSERR_OK, videoDec_->Start());
    EXPECT_EQ(MSERR_OK, videoEnc_->Start());
    sleep(10); // start run 10s
    EXPECT_EQ(MSERR_OK, videoEnc_->NotifyEos());
    EXPECT_EQ(MSERR_OK, videoDec_->Stop());
    EXPECT_EQ(MSERR_OK, videoEnc_->Stop());
    format->Destroy();
}