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

#ifndef AVMETADATAHELPER_DEMO_H
#define AVMETADATAHELPER_DEMO_H

#include <queue>
#include <string_view>
#include "nocopyable.h"
#include "avmetadatahelper.h"

namespace OHOS {
namespace Media {
class AVMetadataHelperDemo : public NoCopyable {
public:
    AVMetadataHelperDemo() = default;
    ~AVMetadataHelperDemo() = default;
    void RunCase(const std::string &pathOuter);

private:
    int32_t SetSource(const std::string &pathOuter);
    void GetMetadata(std::queue<std::string_view> &options);
    void FetchFrame(std::queue<std::string_view> &options);
    void FetchArtPicture(std::queue<std::string_view> &option);
    void DoFetchFrame(int64_t timeUs, int32_t queryOption, const PixelMapParams &param);
    void DoNext();
    std::shared_ptr<AVMetadataHelper> avMetadataHelper_;
};
} // namespace Media
} // namespace OHOS
#endif // AVMETADATAHELPER_DEMO_H
