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
#include "avmetadata_unit_test.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace AVMetadataTestParam;

/**
    Function: compare metadata
    Description: test for metadata
    Input: uri, expected MetaData
    Return: null
*/
void AVMetadataUnitTest::CheckMeta(std::string uri, std::unordered_map<int32_t, std::string> expectMeta)
{
    std::shared_ptr<AVMetadataMock> helper = std::make_shared<AVMetadataMock>();
    ASSERT_NE(nullptr, helper);
    ASSERT_EQ(true, helper->CreateAVMetadataHelper());
    ASSERT_EQ(MSERR_OK, helper->SetSource(uri, 0, 0, AVMetadataUsage::AV_META_USAGE_META_ONLY));
    for (auto &item : expectMeta) {
        std::string value = helper->ResolveMetadata(item.first);
        EXPECT_EQ(AVMetadataTestBase::GetInstance().CompareMetadata(item.first, value, item.second), true);
    }
    auto resultMetas = helper->ResolveMetadata();
    EXPECT_EQ(AVMetadataTestBase::GetInstance().CompareMetadata(resultMetas, expectMeta), true);
    helper->Release();
}

/**
    * @tc.number    : GetThumbnail
    * @tc.name      : Get Thumbnail
    * @tc.desc      : Get THUMBNAIL Function case
*/
void AVMetadataUnitTest::GetThumbnail(const std::string uri)
{
    std::shared_ptr<AVMetadataMock> helper = std::make_shared<AVMetadataMock>();
    ASSERT_NE(nullptr, helper);
    ASSERT_EQ(true, helper->CreateAVMetadataHelper());
    ASSERT_EQ(MSERR_OK, helper->SetSource(uri, 0, 0, AVMetadataUsage::AV_META_USAGE_PIXEL_MAP));

    struct PixelMapParams param = {-1, -1, PixelFormat::RGB_565};
    int64_t timeUs = 0;
    int32_t queryOption = AVMetadataQueryOption::AV_META_QUERY_NEXT_SYNC;
    std::shared_ptr<PixelMap> frame = helper->FetchFrameAtTime(timeUs, queryOption, param);
    ASSERT_NE(nullptr, frame);
    helper->FrameToFile(frame, testInfo_->name(), timeUs, queryOption);
    helper->FrameToJpeg(frame, testInfo_->name(), timeUs, queryOption);
    timeUs = 5000000;  // 5000000us
    frame = helper->FetchFrameAtTime(timeUs, queryOption, param);
    ASSERT_NE(nullptr, frame);
    helper->FrameToFile(frame, testInfo_->name(), timeUs, queryOption);
    helper->FrameToJpeg(frame, testInfo_->name(), timeUs, queryOption);

    param = {-1, -1, PixelFormat::RGB_888};
    frame = helper->FetchFrameAtTime(timeUs, queryOption, param);
    ASSERT_NE(nullptr, frame);
    helper->FrameToFile(frame, testInfo_->name(), timeUs, queryOption);
    helper->FrameToJpeg(frame, testInfo_->name(), timeUs, queryOption);
    timeUs = 0;
    frame = helper->FetchFrameAtTime(timeUs, queryOption, param);
    ASSERT_NE(nullptr, frame);
    helper->FrameToFile(frame, testInfo_->name(), timeUs, queryOption);
    helper->FrameToJpeg(frame, testInfo_->name(), timeUs, queryOption);
    helper->Release();
}

/**
    * @tc.number    : ResolveMetadata_Format_MP4_0100
    * @tc.name      : 01.MP4 format Get MetaData (H264+AAC)
    * @tc.desc      : test ResolveMetadata
*/
HWTEST_F(AVMetadataUnitTest, ResolveMetadata_Format_MP4_0100, TestSize.Level0)
{
    std::unordered_map<int32_t, std::string> expectMeta = {
        {AV_KEY_ALBUM, "media"},
        {AV_KEY_ALBUM_ARTIST, "media_test"},
        {AV_KEY_ARTIST, "元数据测试"},
        {AV_KEY_AUTHOR, ""},
        {AV_KEY_COMPOSER, "测试"},
        {AV_KEY_DURATION, "10030"},
        {AV_KEY_GENRE, "Lyrical"},
        {AV_KEY_HAS_AUDIO, "yes"},
        {AV_KEY_HAS_VIDEO, "yes"},
        {AV_KEY_MIME_TYPE, "video/mp4"},
        {AV_KEY_NUM_TRACKS, "2"},
        {AV_KEY_SAMPLE_RATE, "44100"},
        {AV_KEY_TITLE, "test"},
        {AV_KEY_VIDEO_HEIGHT, "480"},
        {AV_KEY_VIDEO_WIDTH, "720"},
        {AV_KEY_DATE_TIME, "2022-05-29 22:10:43"},
    };
    std::string uri = AVMetadataTestBase::GetInstance().GetMountPath() +
    std::string("/H264_AAC.mp4");
    CheckMeta(uri, expectMeta);
}

/**
    * @tc.number    : FetchFrameAtTime_Resolution_2800
    * @tc.name      : Resolution 480x320
    * @tc.desc      : Get THUMBNAIL
*/
HWTEST_F(AVMetadataUnitTest, FetchFrameAtTime_Resolution_2800, TestSize.Level0)
{
    std::string uri = AVMetadataTestBase::GetInstance().GetMountPath() +
    std::string("/out_480_320.mp4");
    GetThumbnail(uri);
}
