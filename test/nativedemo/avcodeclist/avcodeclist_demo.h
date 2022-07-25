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

#ifndef AVCODECLIST_DEMO_H
#define AVCODECLIST_DEMO_H
#include <functional>
#include <unordered_map>
#include "securec.h"
#include "nocopyable.h"
#include "avcodec_list.h"
#include "format.h"

namespace OHOS {
namespace Media {
class AVCodecListDemo : public NoCopyable {
public:
    AVCodecListDemo() = default;
    virtual ~AVCodecListDemo() = default;

    void RunCase(const std::string &path);
    void DoNext();

private:
    std::shared_ptr<AVCodecList> avCodecList_ = nullptr;
    void PrintVideoCapsArray(const std::vector<std::shared_ptr<VideoCaps>> &videoCapsArray) const;
    void PrintAudioCapsArray(const std::vector<std::shared_ptr<AudioCaps>> &audioCapsArray) const;
    void PrintIntArray(const std::vector<int32_t> &array, const std::string &logmsg) const;
    bool BuildFormat(Format &format);
    void SetMediaDescriptionToFormat(Format &format, const std::string &key);
    void GetSupportedFrameRatesDemo();
    void GetPreferredFrameRateDemo();
    ImgSize SetSize();
};
} // namespace Media
} // namespace OHOS
#endif // AVCODECLIST_DEMO_H
