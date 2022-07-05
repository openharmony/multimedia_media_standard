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

#ifndef TEST_PARAM_COMMON_H
#define TEST_PARAM_COMMON_H

#include <cstdint>
#include <string>
#include "avmetadatahelper.h"
#include "recorder.h"

namespace OHOS {
namespace Media {
namespace PlayerTestParam {
inline constexpr int32_t SEEK_TIME_5_SEC = 5000;
inline constexpr int32_t SEEK_TIME_2_SEC = 2000;
inline constexpr int32_t WAITSECOND = 6;
inline constexpr int32_t DELTA_TIME = 1000;
const std::string MEDIA_ROOT = "file://data/media/";
const std::string VIDEO_FILE1 = MEDIA_ROOT + "test_1920_1080_1.mp4";
} // namespace PlayerTestParam
namespace AVMetadataTestParam {
inline constexpr int32_t PARA_MAX_LEN = 256;
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
class AVMetadataTestBase {
public:
    static AVMetadataTestBase &GetInstance()
    {
        static AVMetadataTestBase config;
        return config;
    }
    std::string GetMountPath() const
    {
        return mountPath_;
    }
    void SetMountPath(std::string mountPath)
    {
        mountPath_ = mountPath;
    }
    bool StrToInt64(const std::string &str, int64_t &value);
    std::string GetPrettyDuration(int64_t duration);
    bool CompareMetadata(int32_t key, const std::string &result, const std::string &expected);
    bool CompareMetadata(const std::unordered_map<int32_t, std::string> &result,
                         const std::unordered_map<int32_t, std::string> &expected);
private:
    AVMetadataTestBase();
    ~AVMetadataTestBase();
    std::string mountPath_ = "file:///data/media/";
};
} // namespace PlAVMetadataTestParamayerTestParam
} // namespace Media
} // namespace OHOS

#endif