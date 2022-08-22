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

#include "avmetadatahelper_demo.h"
#include <iostream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include "jpeglib.h"
#include "string_ex.h"
#include "securec.h"
#include "uri_helper.h"

namespace OHOS {
namespace Media {
static constexpr int32_t RGBA8888_PIXEL_BYTES = 4;
static constexpr int32_t RGB888_PIXEL_BYTES = 3;
static constexpr int32_t RGB565_PIXEL_BYTES = 2;
static constexpr uint16_t RGB565_MASK_RED = 0x001F;
static constexpr uint16_t RGB565_MASK_GREEN = 0x07E0;
static constexpr uint16_t RGB565_MASK_BLUE = 0xF800;
static constexpr uint32_t RGBA8888_MASK_RED = 0x00FF0000;
static constexpr uint32_t RGBA8888_MASK_GREEN = 0x0000FF00;
static constexpr uint32_t RGBA8888_MASK_BLUE = 0x000000FF;
static constexpr uint8_t SHIFT_2_BIT = 2;
static constexpr uint8_t SHITF_3_BIT = 3;
static constexpr uint8_t SHIFT_5_BIT = 5;
static constexpr uint8_t SHIFT_8_BIT = 8;
static constexpr uint8_t SHIFT_11_BIT = 11;
static constexpr uint8_t SHIFT_16_BIT = 16;
static constexpr uint8_t R_INDEX = 2;
static constexpr uint8_t G_INDEX = 1;
static constexpr uint8_t B_INDEX = 0;

#define AVMETA_KEY_TO_STRING_MAP_ITEM(key) { key, #key }
static const std::unordered_map<int32_t, std::string_view> AVMETA_KEY_TO_STRING_MAP = {
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_ALBUM),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_ALBUM_ARTIST),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_ARTIST),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_AUTHOR),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_COMPOSER),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_DATE_TIME),
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_DATE_TIME_FORMAT),
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
    AVMETA_KEY_TO_STRING_MAP_ITEM(AV_KEY_VIDEO_ORIENTATION),
};

static struct jpeg_compress_struct jpeg;
static struct jpeg_error_mgr jerr;

static int32_t Rgb888ToJpeg(const std::string_view &filename, const uint8_t *rgbData, int32_t width, int32_t height)
{
    if (rgbData == nullptr) {
        std::cout << "rgbData is nullptr" << std::endl;
        return -1;
    }

    jpeg.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&jpeg);
    jpeg.image_width = static_cast<uint32_t>(width);
    jpeg.image_height = static_cast<uint32_t>(height);
    jpeg.input_components = RGB888_PIXEL_BYTES;
    jpeg.in_color_space = JCS_RGB;
    jpeg_set_defaults(&jpeg);

    static constexpr int32_t quality = 100;
    jpeg_set_quality(&jpeg, quality, TRUE);

    FILE *file = fopen(filename.data(), "wb");
    if (file == nullptr) {
        jpeg_destroy_compress(&jpeg);
        return 0;
    }

    jpeg_stdio_dest(&jpeg, file);
    jpeg_start_compress(&jpeg, TRUE);
    JSAMPROW rowPointer[1];
    for (uint32_t i = 0; i < jpeg.image_height; i++) {
        rowPointer[0] = const_cast<uint8_t *>(rgbData + i * jpeg.image_width * RGB888_PIXEL_BYTES);
        (void)jpeg_write_scanlines(&jpeg, rowPointer, 1);
    }
    jpeg_finish_compress(&jpeg);
    (void)fclose(file);
    file = nullptr;

    jpeg_destroy_compress(&jpeg);
    return 0;
}

// only valid for little-endian order.
static int32_t RGB565ToRGB888(const uint16_t *rgb565Buf, int32_t rgb565Size, uint8_t *rgb888Buf, int32_t rgb888Size)
{
    if (rgb565Buf == nullptr || rgb565Size <= 0 || rgb888Buf == nullptr || rgb888Size <= 0) {
        return -1;
    }

    if (rgb888Size < rgb565Size * RGB888_PIXEL_BYTES) {
        return -1;
    }

    for (int32_t i = 0; i < rgb565Size; i++) {
        rgb888Buf[i * RGB888_PIXEL_BYTES + R_INDEX] = (rgb565Buf[i] & RGB565_MASK_RED);
        rgb888Buf[i * RGB888_PIXEL_BYTES + G_INDEX] = (rgb565Buf[i] & RGB565_MASK_GREEN) >> SHIFT_5_BIT;
        rgb888Buf[i * RGB888_PIXEL_BYTES + B_INDEX] = (rgb565Buf[i] & RGB565_MASK_BLUE) >> SHIFT_11_BIT;
        rgb888Buf[i * RGB888_PIXEL_BYTES + R_INDEX] <<= SHITF_3_BIT;
        rgb888Buf[i * RGB888_PIXEL_BYTES + G_INDEX] <<= SHIFT_2_BIT;
        rgb888Buf[i * RGB888_PIXEL_BYTES + B_INDEX] <<= SHITF_3_BIT;
    }

    return 0;
}

static int32_t RGBA8888ToRGB888(const uint32_t *rgba8888Buf, int32_t rgba8888Size,
    uint8_t *rgb888Buf, int32_t rgb888Size)
{
    if (rgba8888Buf == nullptr || rgba8888Size <= 0 || rgb888Buf == nullptr || rgb888Size <= 0) {
        return -1;
    }

    if (rgb888Size < rgba8888Size * RGB888_PIXEL_BYTES) {
        return -1;
    }

    for (int32_t i = 0; i < rgba8888Size; i++) {
        rgb888Buf[i * RGB888_PIXEL_BYTES + R_INDEX] = (rgba8888Buf[i] & RGBA8888_MASK_RED) >> SHIFT_16_BIT;
        rgb888Buf[i * RGB888_PIXEL_BYTES + G_INDEX] = (rgba8888Buf[i] & RGBA8888_MASK_GREEN) >> SHIFT_8_BIT;
        rgb888Buf[i * RGB888_PIXEL_BYTES + B_INDEX] = rgba8888Buf[i] & RGBA8888_MASK_BLUE;
    }

    return 0;
}

bool StrToInt64(const std::string &str, int64_t &value)
{
    if (str.empty() || (!isdigit(str.front()) && (str.front() != '-'))) {
        return false;
    }

    char *end = nullptr;
    errno = 0;
    auto addr = str.data();
    auto result = strtoll(addr, &end, 10); /* 10 means decimal */
    if ((end == addr) || (end[0] != '\0') || (errno == ERANGE)) {
        std::cout << "call StrToInt func false, input str is: " << str << std::endl;
        return false;
    }

    value = result;
    return true;
}

std::string_view TrimStr(const std::string_view& str, const char cTrim = ' ')
{
    std::string_view strTmp = str.substr(str.find_first_not_of(cTrim));
    strTmp = strTmp.substr(0, strTmp.find_last_not_of(cTrim) + sizeof(char));
    return strTmp;
}

void MySplitStr(const std::string_view& str, const std::string_view &sep, std::queue<std::string_view> &strs)
{
    std::string_view strTmp = TrimStr(str);
    std::string_view strPart;
    while (true) {
        std::string::size_type pos = strTmp.find(sep);
        if (pos == std::string::npos || sep.empty()) {
            strPart = TrimStr(strTmp);
            if (!strPart.empty()) {
                strs.push(strPart);
            }
            break;
        } else {
            strPart = TrimStr(strTmp.substr(0, pos));
            if (!strPart.empty()) {
                strs.push(strPart);
            }
            strTmp = strTmp.substr(sep.size() + pos, strTmp.size() - sep.size() - pos);
        }
    }
}

void AVMetadataHelperDemo::GetMetadata(std::queue<std::string_view> &options)
{
    if (options.empty()) {
        std::unordered_map<int32_t, std::string> metadataMap = avMetadataHelper_->ResolveMetadata();
        for (const auto &[key, value] : metadataMap) {
            std::string keyPrettyStr = "unknown key";
            if (AVMETA_KEY_TO_STRING_MAP.find(key) != AVMETA_KEY_TO_STRING_MAP.end()) {
                keyPrettyStr = AVMETA_KEY_TO_STRING_MAP.at(key);
            }
            std::cout << "key: " << keyPrettyStr << " metadata: " << value.c_str() << std::endl;
        }
    } else {
        std::string keyStr = std::string(options.front());
        options.pop();

        int32_t key = -1;
        if (!StrToInt(keyStr, key) || key < 0) {
            std::cout << "You need to configure the key parameter  properly" << std::endl;
            return;
        }

        std::string keyPrettyStr = "unknown key";
        if (AVMETA_KEY_TO_STRING_MAP.find(key) != AVMETA_KEY_TO_STRING_MAP.end()) {
            keyPrettyStr = AVMETA_KEY_TO_STRING_MAP.at(key);
        }
        std::string metadata = avMetadataHelper_->ResolveMetadata(key);
        std::cout << "key: " << keyPrettyStr << ", metadata: " << metadata.c_str() << std::endl;
    }
}

static int32_t SaveRGB565Image(const std::shared_ptr<PixelMap> &frame, const std::string_view &filepath)
{
    int32_t rgb888Size = (frame->GetByteCount() / RGB565_PIXEL_BYTES) * RGB888_PIXEL_BYTES;
    uint8_t *rgb888 = new (std::nothrow) uint8_t[rgb888Size];
    if (rgb888 == nullptr) {
        std::cout << "alloc mem failed" << std::endl;
        return -1;
    }
    const uint16_t *rgb565Data = reinterpret_cast<const uint16_t *>(frame->GetPixels());
    int32_t ret = RGB565ToRGB888(rgb565Data, frame->GetByteCount() / RGB565_PIXEL_BYTES, rgb888, rgb888Size);
    if (ret != 0) {
        std::cout << "convert rgb565 to rgb888 failed" << std::endl;
        delete [] rgb888;
        return ret;
    }

    ret = Rgb888ToJpeg(filepath, rgb888, frame->GetWidth(), frame->GetHeight());
    delete [] rgb888;

    return ret;
}

static int32_t SaveRGBA8888Image(const std::shared_ptr<PixelMap> &frame, const std::string_view &filepath)
{
    int32_t rgb888Size = (frame->GetByteCount() / RGBA8888_PIXEL_BYTES) * RGB888_PIXEL_BYTES;
    uint8_t *rgb888 = new (std::nothrow) uint8_t[rgb888Size];
    if (rgb888 == nullptr) {
        std::cout << "alloc mem failed" << std::endl;
        return -1;
    }
    const uint32_t *rgba8888Data = reinterpret_cast<const uint32_t *>(frame->GetPixels());
    int32_t ret = RGBA8888ToRGB888(rgba8888Data, frame->GetByteCount() / RGBA8888_PIXEL_BYTES, rgb888, rgb888Size);
    if (ret != 0) {
        std::cout << "convert rgba8888 to rgb888 failed" << std::endl;
        delete [] rgb888;
        return ret;
    }

    ret = Rgb888ToJpeg(filepath, rgb888, frame->GetWidth(), frame->GetHeight());
    delete [] rgb888;

    return ret;
}

void AVMetadataHelperDemo::DoFetchFrame(int64_t timeUs, int32_t queryOption, const PixelMapParams &param)
{
    std::shared_ptr<PixelMap> frame = avMetadataHelper_->FetchFrameAtTime(timeUs, queryOption, param);
    if (frame == nullptr) {
        std::cout << "Fetch Frame failed" << std::endl;
        return;
    }

    constexpr uint8_t maxFilePathLength = 255;
    char filePath[maxFilePathLength];
    auto ret = sprintf_s(filePath, maxFilePathLength,
        "/data/media/test/time_%" PRIi64 "_option_%d_width_%d_height_%d_color_%d.jpg",
        timeUs, queryOption, param.dstWidth, param.dstHeight, param.colorFormat);
    if (ret <= 0) {
        std::cout << "generate file path failed" << std::endl;
        return;
    }

    if (param.colorFormat == PixelFormat::RGB_565) {
        ret = SaveRGB565Image(frame, filePath);
    } else if (param.colorFormat == PixelFormat::RGBA_8888) {
        ret = SaveRGBA8888Image(frame, filePath);
    } else if (param.colorFormat == PixelFormat::RGB_888) {
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

void AVMetadataHelperDemo::FetchFrame(std::queue<std::string_view> &options)
{
    PixelMapParams param;
    int32_t queryOption = 0;
    int64_t timeUs = 0;

    while (!options.empty()) {
        auto option = options.front();
        options.pop();

        std::queue<std::string_view> group;
        MySplitStr(option, ":", group);

        static const size_t OPTION_GROUP_ITEM_SIZE = 2;
        if (group.size() != OPTION_GROUP_ITEM_SIZE) {
            continue;
        }

        auto name = group.front();
        auto val = group.back();

        if (name.compare("time") == 0) {
            (void)StrToInt64(std::string(val), timeUs);
            continue;
        }

        if (name.compare("option") == 0) {
            (void)StrToInt(std::string(val), queryOption);
            continue;
        }

        if (name.compare("width") == 0) {
            (void)StrToInt(std::string(val), param.dstWidth);
            continue;
        }

        if (name.compare("height") == 0) {
            (void)StrToInt(std::string(val), param.dstHeight);
            continue;
        }

        if (name.compare("color") == 0) {
            if (val.compare("rgb565") == 0) {
                param.colorFormat = PixelFormat::RGB_565;
            }
            if (val.compare("rgb888") == 0) {
                param.colorFormat = PixelFormat::RGB_888;
            }
            if (val.compare("rgba8888") == 0) {
                param.colorFormat = PixelFormat::RGBA_8888;
            }
            continue;
        }
    }
    std::cout << "time: " << timeUs << " option: " << queryOption << " width: " << param.dstWidth
         << " height:" << param.dstHeight << " color: " << static_cast<int32_t>(param.colorFormat) << std::endl;

    DoFetchFrame(timeUs, queryOption, param);
}

void AVMetadataHelperDemo::FetchArtPicture(std::queue<std::string_view> &options)
{
    (void)options;
    auto result = avMetadataHelper_->FetchArtPicture();
    if (result == nullptr) {
        std::cout << "Fetch art picture failed" << std::endl;
        return;
    }

    std::ofstream ofs("/data/media/cover.img");
    if (!ofs.is_open()) {
        std::cout << "open /data/media/cover.img failed" << std::endl;
        return;
    }

    ofs.write(reinterpret_cast<char *>(result->GetBase()), result->GetSize());
    ofs.close();
    std::cout << "save picture to /data/media/cover.img" << std::endl;
}

void AVMetadataHelperDemo::DoNext()
{
    std::string cmd;
    do {
        std::cout << "Enter your step:" << std::endl;
        (void)std::getline(std::cin, cmd);

        std::queue<std::string_view> options;
        MySplitStr(cmd, " ", options);

        if (options.empty()) {
            continue;
        }

        std::string_view funcName = options.front();
        options.pop();

        if (funcName.compare("metadata") == 0) {
            GetMetadata(options);
            continue;
        }

        if (funcName.compare("fetchframe") == 0) {
            FetchFrame(options);
            continue;
        }

        if (funcName.compare("artpicture") == 0) {
            FetchArtPicture(options);
            continue;
        }

        if (funcName.compare("quit") == 0 || funcName.compare("q") == 0) {
            avMetadataHelper_->Release();
            break;
        }
    } while (1);
}

int32_t AVMetadataHelperDemo::SetSource(const std::string &pathOuter)
{
    std::string path;
    if (pathOuter == "") {
        std::cout << "Please enter the video/audio path: " << std::endl;
        (void)getline(std::cin, path);
    } else {
        path = pathOuter;
    }
    std::cout << "Path is " << path << std::endl;

    UriHelper uriHelper(path);
    if (uriHelper.UriType() != UriHelper::URI_TYPE_FILE && !uriHelper.AccessCheck(UriHelper::URI_READ)) {
        std::cout << "Invalid file Path" << std::endl;
        return -1;
    }

    std::string rawFile = uriHelper.FormattedUri();
    rawFile = rawFile.substr(strlen("file://"));
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

    int32_t ret = avMetadataHelper_->SetSource(fd, 0, length, AVMetadataUsage::AV_META_USAGE_PIXEL_MAP);
    if (ret != 0) {
        std::cout << "SetSource fail" << std::endl;
        (void)close(fd);
        return -1;
    }

    (void)close(fd);
    return 0;
}

void AVMetadataHelperDemo::RunCase(const std::string &pathOuter)
{
    avMetadataHelper_ = OHOS::Media::AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (avMetadataHelper_ == nullptr) {
        std::cout << "avMetadataHelper_ is null" << std::endl;
        return;
    }

    if (SetSource(pathOuter) != 0) {
        return;
    }

    DoNext();
}
} // namespace Media
} // namespace OHOS
