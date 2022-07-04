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

#include "avmetadata_mock.h"
#include <iostream>
#include <string_view>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "gtest/gtest.h"
#include "parameter.h"
#include "media_errors.h"
#include "uri_helper.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace AVMetadataUnitBasic;
using namespace AVMetadataTestParam;

namespace AVMetadataUnitBasic {
#define AVMETA_KEY_TO_STRING_MAP_ITEM(key) { key, #key }
static const std::unordered_map<int32_t, std::string_view> AVMETA_KEY_TO_STRING_MAP = {
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_ALBUM),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_ALBUM_ARTIST),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_DATE_TIME),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_ARTIST),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_AUTHOR),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_COMPOSER),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_DURATION),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_GENRE),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_HAS_AUDIO),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_HAS_VIDEO),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_MIME_TYPE),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_NUM_TRACKS),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_SAMPLE_RATE),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_TITLE),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_VIDEO_HEIGHT),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_VIDEO_WIDTH),
};

TestParamsConfig::TestParamsConfig()
{
}

TestParamsConfig::~TestParamsConfig()
{
}

std::string TestParamsConfig::GetUri()
{
    char path[PARA_MAX_LEN] = "file:///data/media/H264_AAC.mp4";
    GetParameter("sys.media.test.stream.path", "file:///data/media/H264_AAC.mp4",
        &path[0], PARA_MAX_LEN);
    UNITTEST_INFO_LOG("PATH : %s", path);
    return path;
}

void TestParamsConfig::InitMountPath()
{
    UNITTEST_INFO_LOG("%s", __FUNCTION__);
    char mountPath[PARA_MAX_LEN] = "null";
    GetParameter("sys.media.test.mount.path", "null", &mountPath[0], PARA_MAX_LEN);
    if (strcmp(mountPath, "null") != 0) {
        AVMetadataUnitBasic::TestParamsConfig::GetInstance().SetMountPath(mountPath);
    } else {
        AVMetadataUnitBasic::TestParamsConfig::GetInstance().SetMountPath("file:///data/media");
    }
}

bool TestParamsConfig::StrToInt64(const std::string &str, int64_t &value)
{
    if (str.empty() || (!isdigit(str.front()) && (str.front() != '-'))) {
        return false;
    }

    char *end = nullptr;
    errno = 0;
    auto addr = str.c_str();
    auto result = strtoll(addr, &end, 10); /* 10 means decimal */
    if (result == 0) {
        return false;
    }
    if ((end == addr) || (end[0] != '\0') || (errno == ERANGE)) {
        UNITTEST_INFO_LOG("call StrToInt func false,  input str is: %s!", str.c_str());
        return false;
    }

    value = result;
    return true;
}

bool TestParamsConfig::CompareMetadata(int32_t key, const std::string &result, const std::string &expected)
{
    std::string keyStr = (AVMETA_KEY_TO_STRING_MAP.count(key) == 0) ?
        std::string(AVMETA_KEY_TO_STRING_MAP.at(key)) : std::to_string(key);

    do {
        if (key == AV_KEY_DURATION) {
            int64_t resultDuration = 0;
            int64_t expectedDuration = 0;
            if (result.compare(expected) == 0) {
                return true;
            }
            if (!StrToInt64(result, resultDuration) || !StrToInt64(expected, expectedDuration)) {
                break;
            }
            if (std::abs(resultDuration - expectedDuration) > 100) { // max allowed time margin is 100ms
                break;
            }
        } else {
            if (result.compare(expected) != 0) {
                break;
            }
        }
        return true;
    } while (0);

    UNITTEST_INFO_LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>[resolve failed] key = %s, result = %s, expected = %s",
        keyStr.c_str(), result.c_str(), expected.c_str());
    return false;
}

bool TestParamsConfig::CompareMetadata(const std::unordered_map<int32_t, std::string> &result,
    const std::unordered_map<int32_t, std::string> &expected)
{
    std::string resultValue;
    bool success = true;

    for (const auto &[key, expectedValue] : expected) {
        if (result.count(key) == 0) {
            resultValue = "";
        } else {
            resultValue = result.at(key);
        }

        success = success && CompareMetadata(key, resultValue, expectedValue);
    }

    return success;
}

std::string TestParamsConfig::GetPrettyDuration(int64_t duration) // ms
{
    static const int32_t msPerSec = 1000;
    static const int32_t msPerMin = 60 * msPerSec;
    static const int32_t msPerHour = 60 * msPerMin;

    int64_t hour = duration / msPerHour;
    int64_t min = (duration % msPerHour) / msPerMin;
    int64_t sec = (duration % msPerMin) / msPerSec;
    int64_t milliSec = duration % msPerSec;

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << hour << ":" // Convert to standard time format
        << std::setw(2) << min << ":"  // Convert to standard time format
        << std::setw(2) << sec << "."  // Convert to standard time format
        << std::setw(3) << milliSec;   // Convert to standard time format

    return oss.str();
}
}

AVMetadataMock::AVMetadataMock()
{
}

AVMetadataMock::~AVMetadataMock()
{
}

bool AVMetadataMock::CreateAVMetadataHelper()
{
    UNITTEST_INFO_LOG("%s", __FUNCTION__);
    avMetadataHelper_ = AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (avMetadataHelper_ == nullptr) {
        return false;
    }
    return true;
}

int32_t AVMetadataMock::SetSource(const std::string &uri, int32_t usage)
{
    UNITTEST_INFO_LOG("%s %s", __FUNCTION__, uri.c_str());
    return avMetadataHelper_->SetSource(uri, usage);
}

int32_t AVMetadataMock::SetSource(const std::string &path, int64_t offset, int64_t size, int32_t usage)
{
    UNITTEST_INFO_LOG("%s %s", __FUNCTION__, path.c_str());
    std::string rawFile = path.substr(strlen("file://"));
    int32_t fd = open(rawFile.c_str(), O_RDONLY);
    if (fd <= 0) {
        std::cout << "Open file failed" << std::endl;
        return -1;
    }

    struct stat64 st;
    if (fstat64(fd, &st) != 0) {
        std::cout << "Get file state failed" << std::endl;
        (void)close(fd);
        return -1;
    }
    int64_t length = static_cast<int64_t>(st.st_size);
    if (size > 0) {
        length = size;
    }
    int32_t ret = avMetadataHelper_->SetSource(fd, offset, length, usage);
    if (ret != 0) {
        (void)close(fd);
        return -1;
    }

    (void)close(fd);
    return ret;
}

void AVMetadataMock::PrintMetadata()
{
    UNITTEST_INFO_LOG("%s", __FUNCTION__);
    auto resultMetas = avMetadataHelper_->ResolveMetadata();
    for (const auto &[key, value]: resultMetas) {
        std::string prettyValue = value;
        if (key == AV_KEY_DURATION) {
            int64_t resultDuration = 0;
            TestParamsConfig::GetInstance().StrToInt64(value, resultDuration);
            prettyValue = TestParamsConfig::GetInstance().GetPrettyDuration(resultDuration);
        }
        if (AVMETA_KEY_TO_STRING_MAP.count(key) != 0) {
            UNITTEST_INFO_LOG("key %s: value %s", AVMETA_KEY_TO_STRING_MAP.at(key).data(), prettyValue.c_str());
        } else {
            UNITTEST_INFO_LOG("key %d: value %s", key, prettyValue.c_str());
        }
    }
}

std::unordered_map<int32_t, std::string> AVMetadataMock::ResolveMetadata()
{
    UNITTEST_INFO_LOG("%s", __FUNCTION__);
    return avMetadataHelper_->ResolveMetadata();
}

std::string AVMetadataMock::ResolveMetadata(int32_t key)
{
    UNITTEST_INFO_LOG("%s(%d)", __FUNCTION__, key);
    return avMetadataHelper_->ResolveMetadata(key);
}

std::shared_ptr<PixelMap> AVMetadataMock::FetchFrameAtTime(int64_t timeUs, int32_t option, PixelMapParams param)
{
    UNITTEST_INFO_LOG("%s", __FUNCTION__);
    return avMetadataHelper_->FetchFrameAtTime(timeUs, option, param);
}

std::shared_ptr<AVSharedMemory> AVMetadataMock::FetchArtPicture()
{
    UNITTEST_INFO_LOG("%s", __FUNCTION__);
    return avMetadataHelper_->FetchArtPicture();
}

void AVMetadataMock::Release()
{
    UNITTEST_INFO_LOG("%s", __FUNCTION__);
    return avMetadataHelper_->Release();
}

// only valid for little-endian order.
int32_t AVMetadataMock::RGB565ToRGB888(const unsigned short *rgb565Buf, int rgb565Size,
    unsigned char *rgb888Buf, int rgb888Size)
{
    if (rgb565Buf == nullptr || rgb565Size <= 0 || rgb888Buf == nullptr || rgb888Size <= 0) {
        return -1;
    }

    if (rgb888Size < rgb565Size * RGB888_PIXEL_BYTES) {
        return -1;
    }

    for (int i = 0; i < rgb565Size; i++) {
        rgb888Buf[i * RGB888_PIXEL_BYTES + R_INDEX] = (rgb565Buf[i] & RGB565_MASK_RED);
        rgb888Buf[i * RGB888_PIXEL_BYTES + G_INDEX] = (rgb565Buf[i] & RGB565_MASK_GREEN) >> SHIFT_5_BIT;
        rgb888Buf[i * RGB888_PIXEL_BYTES + B_INDEX] = (rgb565Buf[i] & RGB565_MASK_BLUE) >> SHIFT_11_BIT;
        rgb888Buf[i  * RGB888_PIXEL_BYTES + R_INDEX] <<= SHIFT_3_BIT;
        rgb888Buf[i  * RGB888_PIXEL_BYTES + G_INDEX] <<= SHIFT_2_BIT;
        rgb888Buf[i  * RGB888_PIXEL_BYTES + B_INDEX] <<= SHIFT_3_BIT;
    }

    return 0;
}

int32_t AVMetadataMock::Rgb888ToJpeg(const std::string_view &filename,
    const uint8_t *rgbData, int width, int height)
{
    if (rgbData == nullptr) {
        std::cout << "rgbData is nullptr" << std::endl;
        return -1;
    }

    jpeg.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&jpeg);
    jpeg.image_width = width;
    jpeg.image_height = height;
    jpeg.input_components = RGB888_PIXEL_BYTES;
    jpeg.in_color_space = JCS_RGB;
    jpeg_set_defaults(&jpeg);

    static const int QUALITY = 100;
    jpeg_set_quality(&jpeg, QUALITY, TRUE);

    FILE *pFile = fopen(filename.data(), "wb");
    if (!pFile) {
        jpeg_destroy_compress(&jpeg);
        return 0;
    }

    jpeg_stdio_dest(&jpeg, pFile);
    jpeg_start_compress(&jpeg, TRUE);
    JSAMPROW row_pointer[1];
    for (uint32_t i = 0; i < jpeg.image_height; i++) {
        row_pointer[0] = const_cast<uint8_t *>(rgbData + i * jpeg.image_width * RGB888_PIXEL_BYTES);
        jpeg_write_scanlines(&jpeg, row_pointer, 1);
    }
    jpeg_finish_compress(&jpeg);
    (void)fclose(pFile);
    pFile = NULL;

    jpeg_destroy_compress(&jpeg);
    return 0;
}

void AVMetadataMock::FrameToFile(std::shared_ptr<PixelMap> frame,
    const char *fileName, int64_t timeUs, int32_t queryOption)
{
    const uint8_t *data = frame->GetPixels();
    EXPECT_NE(data, nullptr);
    int32_t bufferSize = frame->GetByteCount();
    const uint8_t MAX_FILE_PATH_LENGTH = 255;
    char filePath[MAX_FILE_PATH_LENGTH];
    if (access("/data/media/ThumbnailBak", 0) != F_OK) {
        mkdir("/data/media/ThumbnailBak", 0777);  // permission
    }
    auto ret = sprintf_s(filePath, MAX_FILE_PATH_LENGTH,
        "/data/media/ThumbnailBak/%s_time_%" PRIi64 "_option_%d_width_%d_height_%d_color_%d.pixel",
        fileName, timeUs, queryOption, frame->GetWidth(), frame->GetHeight(), frame->GetPixelFormat());
    if (ret <= 0) {
        std::cout << "generate file path failed" << std::endl;
        return;
    }
    FILE *desFile = fopen(filePath, "wb");
    ASSERT_NE(desFile, nullptr);
    int64_t dstBufferSize = fwrite(data, bufferSize, 1, desFile);
    EXPECT_EQ(dstBufferSize, 1);
    (void)fclose(desFile);
}

void AVMetadataMock::SurfaceToFile(std::shared_ptr<AVSharedMemory> frame,
    const char *fileName)
{
    const uint8_t *data = frame->GetBase();
    EXPECT_NE(data, nullptr);
    int32_t bufferSize = frame->GetSize();
    const uint8_t MAX_FILE_PATH_LENGTH = 255;
    char filePath[MAX_FILE_PATH_LENGTH];
    if (access("/data/media/SurfaceBak", 0) != F_OK) {
        mkdir("/data/media/SurfaceBak", 0777); // permission 777
    }
    auto ret = sprintf_s(filePath, MAX_FILE_PATH_LENGTH, "/data/media/SurfaceBak/%s.pixel", fileName);
    if (ret <= 0) {
        std::cout << "generate file path failed" << std::endl;
        return;
    }
    FILE *desFile = fopen(filePath, "wb");
    ASSERT_NE(desFile, nullptr);
    int64_t dstBufferSize = fwrite(data, bufferSize, 1, desFile);
    EXPECT_EQ(dstBufferSize, 1);
    (void)fclose(desFile);
}

void AVMetadataMock::FrameToJpeg(std::shared_ptr<PixelMap> frame,
    const char *fileName, int64_t timeUs, int32_t queryOption)
{
    const uint8_t MAX_FILE_PATH_LENGTH = 255;
    char filePath[MAX_FILE_PATH_LENGTH];
    if (access("/data/media/ThumbnailBak", 0) != F_OK) {
        mkdir("/data/media/ThumbnailBak", 0777); // permission 777
    }
    auto ret = sprintf_s(filePath, MAX_FILE_PATH_LENGTH,
        "/data/media/ThumbnailBak/%s_time_%" PRIi64 "_option_%d_width_%d_height_%d_color_%d.jpg",
        fileName, timeUs, queryOption, frame->GetWidth(), frame->GetHeight(), frame->GetPixelFormat());
    if (ret <= 0) {
        std::cout << "generate file path failed" << std::endl;
        return;
    }
    if (frame->GetPixelFormat() == PixelFormat::RGB_565) {
        uint32_t rgb888Size = (frame->GetByteCount() / RGB565_PIXEL_BYTES) * RGB888_PIXEL_BYTES;
        uint8_t *rgb888 = new (std::nothrow) uint8_t[rgb888Size];
        if (rgb888 == nullptr) {
            std::cout << "alloc mem failed" << std::endl;
            return;
        }
        const uint16_t *rgb565Data = reinterpret_cast<const uint16_t *>(frame->GetPixels());
        ret = RGB565ToRGB888(rgb565Data, frame->GetByteCount() / RGB565_PIXEL_BYTES, rgb888, rgb888Size);
        if (ret != 0) {
            std::cout << "convert rgb565 to rgb888 failed" << std::endl;
            delete [] rgb888;
            return;
        }
        ret = Rgb888ToJpeg(filePath, rgb888, frame->GetWidth(), frame->GetHeight());
        delete [] rgb888;
    } else if (frame->GetPixelFormat() == PixelFormat::RGB_888) {
        ret = Rgb888ToJpeg(filePath, frame->GetPixels(), frame->GetWidth(), frame->GetHeight());
    } else {
        std::cout << "invalid pixel format" << std::endl;
        return;
    }
    if (ret != 0) {
        std::cout << "pack image failed" << std::endl;
    }
    std::cout << "save to " << filePath << std::endl;
}